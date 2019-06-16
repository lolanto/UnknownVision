#include "DX12ResourceManager.h"
#include <algorithm>

BEG_NAME_SPACE
DX12ResourceManager::DX12ResourceManager(DX12ResourceManager && rhs)
{
	swapWithRValue(std::move(rhs));
}

DX12ResourceManager & DX12ResourceManager::operator=(DX12ResourceManager && rhs)
{
	swapWithRValue(std::move(rhs));
	return *this;
}

inline void DX12ResourceManager::swapWithRValue(DX12ResourceManager&& rhs) {
	m_device = rhs.m_device;
	m_uploadHeapMgr.swap(rhs.m_uploadHeapMgr);
	m_defaultHeapMgr.swap(rhs.m_defaultHeapMgr);
	m_buffers.swap(rhs.m_buffers);
	m_addrIndex.swap(rhs.m_addrIndex);
}

auto DX12ResourceManager::genNewBuffer(size_t size, ResourceUsage usage, ResourceFlag flag)
	->std::pair<ID3D12Resource*, ResourceState> thread_safe {

	/** TODO: 不是所有的USAGE都兼顾！ */
	D3D12_RESOURCE_FLAGS resFlag = D3D12_RESOURCE_FLAG_NONE;
	if (usage & RESOURCE_USAGE_UNORDER_ACCESS) resFlag |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (!(usage & RESOURCE_USAGE_SHADER_RESOURCE)) resFlag |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (usage & RESOURCE_USAGE_RENDER_TARGET) resFlag |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size, resFlag);
	BufferInfo info;
	info.status.usage = usage;
	info.status.flag = flag;
	info.size = size;
	info.isFree = false;

	/** 所有需要CPU读取的缓冲都使用committed resource */
	if (flag == RESOURCE_FLAG_READ_BACK) {
		if (FAILED(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(&info.buffer))))
			return { nullptr, RESOURCE_STATE_INVALID };
		info.status.state = RESOURCE_STATE_COPY_DEST;
	}
	/** 一个资源是commited resource或是placed resource应该由资源的大小决定 */
	else if (size >= BUFFER_SIZE_USED_COMMITTED_RESOURCE) {
		/** 使用committed resource创建该资源 */
		if (FAILED(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(placedBufferOnUploadHeap(flag) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(&info.buffer))))
			return { nullptr, RESOURCE_STATE_INVALID };
		info.status.state = RESOURCE_STATE_COPY_DEST;
	}
	else {
		/** 使用placed resource创建该资源 */
		auto allocInfo = m_device->GetResourceAllocationInfo(0, 1, &bufferDesc);
		auto[heap, blockInfo] = placedBufferOnUploadHeap(flag) ?
			m_uploadHeapMgr->RequestBlock(allocInfo.SizeInBytes, allocInfo.Alignment) :
			m_defaultHeapMgr->RequestBlock(allocInfo.SizeInBytes, allocInfo.Alignment);
		if (FAILED(m_device->CreatePlacedResource(heap,
			blockInfo.offset % HEAP_CHUNK_SIZE, &bufferDesc,
			placedBufferOnUploadHeap(flag) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr, IID_PPV_ARGS(&info.buffer))))
			return { nullptr, RESOURCE_STATE_INVALID };
		info.status.state = placedBufferOnUploadHeap(flag) ? RESOURCE_STATE_GENERIC_READ : RESOURCE_STATE_COPY_DEST;
		info.onUpload = placedBufferOnUploadHeap(flag);
		info.info = blockInfo;
	}

	{
		std::lock_guard<std::mutex> lg(m_bufferLock);
		auto iter = m_buffers.insert(info);
		m_addrIndex.insert(std::make_pair(reinterpret_cast<std::uintptr_t>(info.buffer.Get()), iter));
	}
	return { info.buffer.Get(), info.status.state };
}

auto DX12ResourceManager::findoutFreeBuffer(size_t size, ResourceUsage usage, ResourceFlag flag)
	->std::pair<ID3D12Resource*, ResourceState> thread_safe {
	
	std::lock_guard<std::mutex> lg(m_bufferLock);
	auto candidate = m_buffers.lower_bound(size); /**< 候选的buffer大小必须大于等于申请的大小 */
	for (; candidate != m_buffers.end(); ++candidate) {
		const auto& candValue = *candidate;
		if (candValue.isFree == false) continue; /**< 当前没有被占用 */
		if (candValue.status.flag != flag) continue; /**< 特性完全符合 */
		if ((candValue.status.usage & usage) != usage) continue; /**< 候选的buffer必须能够满足申请的所有用途 */
		break;
	}
	if (candidate != m_buffers.end()) {
		candidate->isFree = false;
		return { candidate->buffer.Get(), candidate->status.state };
	}
	return { nullptr, RESOURCE_STATE_INVALID };
}

auto DX12ResourceManager::RequestBuffer(size_t size, ResourceUsage usage,
	ResourceFlag flag) -> std::pair<ID3D12Resource*, ResourceState> thread_safe {
	auto res = findoutFreeBuffer(size, usage, flag);
	if (res.first == nullptr) res = genNewBuffer(size, usage, flag);
	return res;
}

bool DX12ResourceManager::RevertBuffer(ID3D12Resource* res, ResourceState state) thread_safe {
	
	std::lock_guard<std::mutex> lg(m_bufferLock);
	auto iter = m_addrIndex.find(reinterpret_cast<uintptr_t>(res));
	if (iter == m_addrIndex.end()) return false;
	/** 将归还的buffer的占用状态以及当前使用状态进行修改和记录 */
	iter->second->status.state = state;
	iter->second->isFree = true;
	return true;
}

void DX12ResourceManager::flush() thread_safe {

	std::lock_guard<std::mutex> lg(m_bufferLock);
	for (auto indexIter = m_addrIndex.begin();
		indexIter != m_addrIndex.end();) {
		if (indexIter->second->isFree) {
			auto& bufferInfo = *indexIter->second;
			bufferInfo.buffer.Reset(); /**< 释放com对象 */
			if (bufferInfo.info.size) { /**< 假如是从堆上申请的空间，则释放堆空间 */
				if (bufferInfo.onUpload) m_uploadHeapMgr->RevertBlock(bufferInfo.info);
				else m_defaultHeapMgr->RevertBlock(bufferInfo.info);
			}
			m_buffers.erase(indexIter->second);
			indexIter = m_addrIndex.erase(indexIter);
		}
		else {
			++indexIter;
		}
	}

}

END_NAME_SPACE
