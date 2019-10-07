#include "DX12DescriptorHeap.h"
#include <assert.h>
#include <limits>
#include <D3D12MemAlloc.h>
BEG_NAME_SPACE

bool BasicDX12DescriptorHeap::Initialize(ID3D12Device * dev, D3D12_DESCRIPTOR_HEAP_TYPE type,
	size_t numberOfDescriptors, bool bShaderVisibility)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && bShaderVisibility) ?
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	desc.NumDescriptors = numberOfDescriptors;
	desc.Type = type;
	assert(SUCCEEDED(dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descHeap))));
	m_desc = desc;
	m_cpu_beg = m_descHeap->GetCPUDescriptorHandleForHeapStart();
	if (desc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		m_gpu_beg = m_descHeap->GetGPUDescriptorHandleForHeapStart();
	else
		m_gpu_beg.ptr = std::numeric_limits<decltype(m_gpu_beg.ptr)>::max();
	return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DiscretePermanentDX12DescriptorHeap::RequestBlock()
{
	/** 不允许超出分配范围 */
	assert(m_nextHead < m_capacity);
	size_t res = m_nextHead++;
	return GetCPUHandle(res);
}

void DiscretePermanentDX12DescriptorHeap::Release(D3D12_CPU_DESCRIPTOR_HANDLE head)
{
}

size_t LocalDynamicDX12DescriptorHeap::BeginAllocation()
{
	assert(m_bBeginAllocate == false); /**< 再次请求时上次的分配必须结束 */
	m_nextHead = m_begin;
	m_bBeginAllocate = true;
	return m_begin;
}

size_t LocalDynamicDX12DescriptorHeap::RequestBlock(size_t size, ID3D12Resource ** pResources)
{
	/** 对请求的资源进行hash，在hash表中寻找是否已经分配过，若是则直接返回之前的记录
	 * 否则再创建一个新的块 */
	size_t res = m_nextHead;
	m_nextHead += size;
}

void LocalDynamicDX12DescriptorHeap::EndAllocation()
{
	assert(m_bBeginAllocate == true);
	AllocatedBlockInfo info;
	info.head = m_begin;
	info.size = m_nextHead - m_begin;
	m_allocatedBlocks.push_back(info);
	m_begin = m_nextHead;
	m_bBeginAllocate = false;
}

void LocalDynamicDX12DescriptorHeap::Release(size_t head)
{
	assert(m_allocatedBlocks.empty() == false); /**< 没有分配记录，不允许调用释放 */
	bool bDetected = false;
	size_t index = 0;
	for (auto record = m_allocatedBlocks.begin();
		record != m_allocatedBlocks.end();
		++record, ++index) {
		if (record->head == head) {
			if (index == 0) {
				/** 假如释放的是第一条记录 */
				if (m_allocatedBlocks.size() == 1) {
					/** 假如只有一条记录，则当前环为空，首位相接 */
					m_end = m_begin;
				}
				else {
					/** 假如还有分配记录，则末尾指向第二老的记录开头 */
					m_end = (record + 1)->head;
				}
			}
			else {
				/** 这是防止可能不按申请顺序释放块，一旦如此，记录会被消除，但除非之前的记录都被清楚，否则该块的表现依然和占用相同 */
				m_allocatedBlocks.erase(record);
			}
			bDetected = true;
			break;
		}
	}
	assert(bDetected);
}

END_NAME_SPACE


