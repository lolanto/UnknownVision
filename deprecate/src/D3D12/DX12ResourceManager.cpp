#include "DX12ResourceManager.h"
#include <algorithm>
#include <assert.h>

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
	m_bufferAddrIndex.swap(rhs.m_bufferAddrIndex);
}

auto DX12ResourceManager::genNewBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType)
	->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe {
	/** 暂不支持custom类型 */
	assert(heapType != D3D12_HEAP_TYPE_CUSTOM);
	
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
<<<<<<< Updated upstream:src/D3D12/DX12ResourceManager.cpp
	auto allocInfo = m_device->GetResourceAllocationInfo(0, 1, &bufferDesc);
=======
>>>>>>> Stashed changes:deprecate/src/D3D12/DX12ResourceManager.cpp
	SmartPTR<ID3D12Resource> buffer;
	D3D12_RESOURCE_STATES state = heapType == D3D12_HEAP_TYPE_UPLOAD ? 
		D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;;

	/** 所有需要CPU读取的缓冲都使用committed resource
	 * 缓冲区大小超过committed resource创建标准的也使用committed resource*/
	if (heapType == D3D12_HEAP_TYPE_READBACK 
		|| allocInfo.SizeInBytes >= BUFFER_SIZE_USED_COMMITTED_RESOURCE) {
		if (FAILED(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(heapType),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc, state, nullptr,
			IID_PPV_ARGS(&buffer))))
			return { nullptr, D3D12_RESOURCE_STATE_COMMON };
	}
	else {
		/** 使用placed resource创建该资源 */
		assert(allocInfo.SizeInBytes <= HEAP_BLOCK_LIMIT); /**< 一次不能申请超过4G */
		auto[heap, blockInfo] = (heapType == D3D12_HEAP_TYPE_UPLOAD) ?
			m_uploadHeapMgr->RequestBlock(allocInfo.SizeInBytes, static_cast<HeapBlockAlignment>(allocInfo.Alignment)) :
			m_defaultHeapMgr->RequestBlock(allocInfo.SizeInBytes, static_cast<HeapBlockAlignment>(allocInfo.Alignment));
		assert(heap != nullptr);
		if (FAILED(m_device->CreatePlacedResource(heap,
			blockInfo.offset % HEAP_CHUNK_SIZE, &bufferDesc,
			state, nullptr, IID_PPV_ARGS(&buffer))))
			return { nullptr, D3D12_RESOURCE_STATE_COMMON };
		bInfo = blockInfo;
	}
	BufferInfo info(std::move(buffer), bInfo, state, flags, heapType, size, false);
	ID3D12Resource* ptr = info.buffer.Get();
	{
		if (info.block.size) {
			if (info.heapType == D3D12_HEAP_TYPE_UPLOAD) {
				FLOG("new buffer upload info: size=%zu, offset=%zu\n", info.block.size, info.block.offset);
			}
			else {
				FLOG("new buffer default info: size=%zu, offset=%zu\n", info.block.size, info.block.offset);
			}
		}
		std::lock_guard<std::mutex> lg(m_bufferLock);
		auto iter = m_buffers.insert(std::move(info));
		m_bufferAddrIndex.insert(std::make_pair(reinterpret_cast<std::uintptr_t>(ptr), iter));
	}
	return { ptr, info.state };
}

<<<<<<< Updated upstream:src/D3D12/DX12ResourceManager.cpp
auto DX12ResourceManager::findoutFreeBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType)
	->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe {
	/** 暂不支持custom类型 */
	assert(heapType != D3D12_HEAP_TYPE_CUSTOM);

	std::lock_guard<std::mutex> lg(m_bufferLock);
	auto candidate = m_buffers.lower_bound(size); /**< 候选的buffer大小必须大于等于申请的大小 */
	for (; candidate != m_buffers.end(); ++candidate) {
		const auto& candValue = *candidate;
		if (candValue.isFree == false) continue; /**< 当前没有被占用 */
		if (candValue.heapType!= heapType) continue; /**< 所在堆栈类型需要相符 */
		if ((candValue.flags & flags) != flags) continue; /**< 特性完全符合 */
		break;
	}
	if (candidate != m_buffers.end()) {
		candidate->isFree = false;
		if (candidate->block.size) {
			if (candidate->heapType == D3D12_HEAP_TYPE_UPLOAD) {
				FLOG("old buffer upload info: size=%zu, offset=%zu\n", candidate->block.size, candidate->block.offset);
			}
			else {
				FLOG("old buffer default info: size=%zu, offset=%zu\n", candidate->block.size, candidate->block.offset);
			}
		}
		return { candidate->buffer.Get(), candidate->state };
	}
	return { nullptr, D3D12_RESOURCE_STATE_COMMON };
}

auto DX12ResourceManager::genNewTexture(uint32_t width, uint32_t height, DXGI_FORMAT elementFormat,
	D3D12_RESOURCE_FLAGS flags, uint16_t mipLevel)
	-> std::pair<ID3D12Resource *, D3D12_RESOURCE_STATES> thread_safe
{

	auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(elementFormat, width, height, 1, mipLevel, 1, 0, flags);
	auto allocInfo = m_device->GetResourceAllocationInfo(0, 1, &texDesc);
=======
	m_resourceRepository.insert({ newResource.pResource, newResource });
	return { newResource.pResource, state };
}

auto DX12ResourceManager::RequestTexture(uint32_t width, uint32_t height, uint32_t slice, DXGI_FORMAT elementFormat,
	D3D12_RESOURCE_FLAGS flags,
	uint16_t mipLevel, bool bForceCommitted)->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> {
	CD3DX12_RESOURCE_DESC texDesc;
	if (slice)
		texDesc = CD3DX12_RESOURCE_DESC::Tex3D(elementFormat, width, height, slice, mipLevel, flags);
	else if (height)
		texDesc = CD3DX12_RESOURCE_DESC::Tex2D(elementFormat, width, height, 1, mipLevel, 1, 0, flags);
	else if (width)
		texDesc = CD3DX12_RESOURCE_DESC::Tex1D(elementFormat, width, 1, mipLevel, flags);
	else
		return { nullptr, D3D12_RESOURCE_STATE_COMMON };

>>>>>>> Stashed changes:deprecate/src/D3D12/DX12ResourceManager.cpp
	SmartPTR<ID3D12Resource> texture;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
	DX12ResourceHeapManager::BlockInfo bInfo;

	if (allocInfo.SizeInBytes >= TEXTURE_SIZE_USED_COMMITTED_RESOURCE) {
		if (FAILED(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&texDesc, state, nullptr, IID_PPV_ARGS(&texture))))
			return { nullptr, D3D12_RESOURCE_STATE_COMMON };
	}
	else {
		/** 使用placed资源创建 */
		assert(allocInfo.SizeInBytes <= HEAP_BLOCK_LIMIT); /**< 一次不能申请超过4G */
		auto[heap, blockInfo] = 
			m_defaultHeapMgr->RequestBlock(allocInfo.SizeInBytes, static_cast<HeapBlockAlignment>(allocInfo.Alignment));
		assert(heap != nullptr);
		if (FAILED(m_device->CreatePlacedResource(heap,
			blockInfo.offset % HEAP_CHUNK_SIZE, &texDesc,
			state, nullptr, IID_PPV_ARGS(&texture))))
			return { nullptr, D3D12_RESOURCE_STATE_COMMON };
		bInfo = blockInfo;
	}
	TextureInfo info(std::move(texture), bInfo, state, flags, elementFormat, width, height, mipLevel, false);
	ID3D12Resource* ptr = info.texture.Get();
	{
		if (info.block.size) {
			FLOG("new texture heap info: size=%zu, offset=%zu\n", info.block.size, info.block.offset);
		}
		std::lock_guard<std::mutex> lg(m_textureLock);
		auto iter = m_textures.insert(std::move(info));
		m_textureAddrIndex.insert(std::make_pair(reinterpret_cast<std::uintptr_t>(ptr), iter));
	}
	return { ptr, state };
}

<<<<<<< Updated upstream:src/D3D12/DX12ResourceManager.cpp
auto DX12ResourceManager::findoutFreeTexture(uint32_t width, uint32_t height, DXGI_FORMAT elementFormat,
	D3D12_RESOURCE_FLAGS flags, uint16_t mipLevel)
	-> std::pair<ID3D12Resource *, D3D12_RESOURCE_STATES> thread_safe
{
	std::lock_guard<std::mutex> lg(m_textureLock);
	auto candidate = m_textures.lower_bound(width, height);
	for (; candidate != m_textures.end(); ++candidate) {
		const auto& candValue = *candidate;
		if (candValue.isFree == false) continue;
		if (candValue.elementFormat != elementFormat) continue;
		if ((candValue.flags & flags) != flags) continue;
		if (candValue.mipLevel != mipLevel) continue;
		break;
=======
bool DX12ResourceManager::ReleaseResource(ID3D12Resource* pRes) {
	auto target = m_resourceRepository.find(pRes);
	if (target != m_resourceRepository.end()) {
		target->first->Release();
		target->second.pAllocation->Release();
		m_resourceRepository.erase(target);
		return true;
>>>>>>> Stashed changes:deprecate/src/D3D12/DX12ResourceManager.cpp
	}
	if (candidate != m_textures.end()) {
		candidate->isFree = false;
		if (candidate->block.size) {
			FLOG("old texture heap info: size=%zu, offset=%zu\n", candidate->block.size, candidate->block.offset);
		}
		return { candidate->texture.Get(), candidate->state };
	}
	return { nullptr, D3D12_RESOURCE_STATE_COMMON };
}

auto DX12ResourceManager::RequestBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
	D3D12_HEAP_TYPE heapType) -> std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe {
	auto res = findoutFreeBuffer(size, flags, heapType);
	if (res.first == nullptr) res = genNewBuffer(size, flags, heapType);
	return res;
}

bool DX12ResourceManager::RevertBuffer(ID3D12Resource* res, D3D12_RESOURCE_STATES state) thread_safe {
	
	std::lock_guard<std::mutex> lg(m_bufferLock);
	auto iter = m_bufferAddrIndex.find(reinterpret_cast<uintptr_t>(res));
	if (iter == m_bufferAddrIndex.end()) return false;
	/** 将归还的buffer的占用状态以及当前使用状态进行修改和记录 */
	iter->second->state = state;
	iter->second->isFree = true;
	if (iter->second->block.size)
		FLOG("rev buffer info: size=%zu, offset=%zu\n", iter->second->block.size, iter->second->block.offset);
	return true;
}

auto DX12ResourceManager::RequestTexture(uint32_t width, uint32_t height, DXGI_FORMAT elementFormat,
	D3D12_RESOURCE_FLAGS flags,
	uint16_t mipLevel) -> std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> thread_safe {
	auto res = findoutFreeTexture(width, height, elementFormat, flags, mipLevel);
	if (res.first == nullptr) res = genNewTexture(width, height, elementFormat, flags, mipLevel);
	return res;
}

bool DX12ResourceManager::RevertTexture(ID3D12Resource* res, D3D12_RESOURCE_STATES state) thread_safe {
	std::lock_guard<std::mutex> lg(m_textureLock);
	auto iter = m_textureAddrIndex.find(reinterpret_cast<uintptr_t>(res));
	if (iter == m_textureAddrIndex.end()) return false;
	/** 将归还的纹理的占用状态以及当前使用状态进行修改和记录 */
	iter->second->state = state;
	iter->second->isFree = true;
	if (iter->second->block.size)
		FLOG("rev texture info: size=%zu, offset=%zu\n", iter->second->block.size, iter->second->block.offset);
	return true;
}

void DX12ResourceManager::flush() thread_safe {

	{
		std::lock_guard<std::mutex> lg(m_bufferLock);
		for (auto indexIter = m_bufferAddrIndex.begin();
			indexIter != m_bufferAddrIndex.end();) {
			if (indexIter->second->isFree) {
				auto& bufferInfo = *indexIter->second;
				bufferInfo.buffer.Reset(); /**< 释放com对象 */
				if (bufferInfo.block.size) { /**< 假如是从堆上申请的空间，则释放堆空间 */
					if (bufferInfo.heapType == D3D12_HEAP_TYPE_UPLOAD) m_uploadHeapMgr->RevertBlock(bufferInfo.block);
					else m_defaultHeapMgr->RevertBlock(bufferInfo.block);
				}
				m_buffers.erase(indexIter->second);
				indexIter = m_bufferAddrIndex.erase(indexIter);
			}
			else {
				++indexIter;
			}
		}
	}
	{
		std::lock_guard<std::mutex> lg(m_textureLock);
		for (auto indexIter = m_textureAddrIndex.begin();
			indexIter != m_textureAddrIndex.end();) {
			if (indexIter->second->isFree) {
				auto& textureInfo = *indexIter->second;
				textureInfo.texture.Reset();
				if (textureInfo.block.size) {
					m_defaultHeapMgr->RevertBlock(textureInfo.block);
				}
				m_textures.erase(indexIter->second);
				indexIter = m_textureAddrIndex.erase(indexIter);
			}
			else {
				++indexIter;
			}
		}
	}
}

void DX12ResourceManager::Statistics() {
	char outputInfo[128] = { 0 };
	size_t totalFreeResourceSize = 0;
	size_t totalUsedResourceSize = 0;
	size_t totalUploadHeapSize = 0;
	size_t totalDefaultHeapSize = 0;
	for (const auto& bufferInfo : m_buffers.buffers) {
		if (bufferInfo.isFree) totalFreeResourceSize += bufferInfo.size;
		else totalUsedResourceSize += bufferInfo.size;
		if (bufferInfo.block.size) {
			if (bufferInfo.heapType == D3D12_HEAP_TYPE_UPLOAD) totalUploadHeapSize += bufferInfo.block.size;
			else totalDefaultHeapSize += bufferInfo.block.size;
		}
	}
	sprintf(outputInfo, "req buffer size is : %zu, free buffer size is : %zu, used buffer size is : %zu \n",
		totalFreeResourceSize + totalUsedResourceSize, totalFreeResourceSize, totalUsedResourceSize);
	MLOG(outputInfo);
	totalFreeResourceSize = 0;
	totalUsedResourceSize = 0;
	for (const auto& textureInfo : m_textures.textures) {
		size_t reqSize = textureInfo.width * textureInfo.height;
		if (textureInfo.elementFormat >= DXGI_FORMAT_R8G8_TYPELESS) assert(false);
		else if (textureInfo.elementFormat >= DXGI_FORMAT_R10G10B10A2_TYPELESS) reqSize *= 4;
		else if (textureInfo.elementFormat >= DXGI_FORMAT_R32G32B32A32_TYPELESS) reqSize *= 16;
		else assert(false);
		if (textureInfo.isFree) totalFreeResourceSize += reqSize;
		else totalUsedResourceSize += reqSize;
		if (textureInfo.block.size) {
			totalDefaultHeapSize += reqSize;
		}
	}
	sprintf(outputInfo, "req texture size is : %zu, free texture size is : %zu, used texture size is : %zu \n",
		totalFreeResourceSize + totalUsedResourceSize, totalFreeResourceSize, totalUsedResourceSize);
	MLOG(outputInfo);
	sprintf(outputInfo, "used upload heap size: %zu, used default heap size: %zu \n", totalUploadHeapSize, totalDefaultHeapSize);
	MLOG(outputInfo);
	sprintf(outputInfo, "upload heap: \n");
	MLOG(outputInfo);
	m_uploadHeapMgr->Statistics();
	sprintf(outputInfo, "default heap: \n");
	MLOG(outputInfo);
	m_defaultHeapMgr->Statistics();
}

END_NAME_SPACE
