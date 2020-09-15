#include "DX12DescriptorHeap.h"
#include <assert.h>
#include <limits>
#include <D3D12MemAlloc.h>
BEG_NAME_SPACE

bool BasicDX12DescriptorHeap::Initialize(ID3D12Device * dev, size_t nodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type,
	size_t numberOfDescriptors, bool bShaderVisibility)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && bShaderVisibility) ?
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = nodeMask;
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

AllocateRange LocalDynamicDX12DescriptorHeap::RequestBlock(
	size_t capacity, size_t completedFenceValue)
{
	AllocateRange retRange; retRange.beg = 1; retRange.end = 0;
	updateFreeRanges(completedFenceValue);
	auto& freeRanges = m_freeRanges.lower_bound(capacity);
	if (freeRanges == m_freeRanges.end() || freeRanges->second.empty() == true) {
		return retRange;
	}
	retRange = freeRanges->second.back();
	freeRanges->second.pop_back();
	m_begToEnd.erase(retRange.beg);
	m_endToBeg.erase(retRange.end);
	if (retRange.AllocatedSize() == capacity) return retRange;
	AllocateRange restRange;
	restRange.beg = retRange.beg + capacity;
	restRange.end = retRange.end;
	retRange.end = retRange.beg + capacity - 1;
	m_freeRanges[restRange.AllocatedSize()].push_front(restRange);
	m_begToEnd.insert({ restRange.beg, restRange.end });
	m_endToBeg.insert({ restRange.end, restRange.beg });
	
	retRange.cpuHandle = GetCPUHandle(retRange.beg);
	retRange.gpuHandle = GetGPUHandle(retRange.beg);
	return retRange;
}

void LocalDynamicDX12DescriptorHeap::ReleaseBlock(AllocateRange range, size_t lastFenceValue)
{
	FenceValueToAllocateRange record;
	record.first = lastFenceValue;
	record.second = range;
	m_allocatedRanges.push(record);
}

void LocalDynamicDX12DescriptorHeap::updateFreeRanges(size_t completedFenceValue)
{
	while (m_allocatedRanges.empty() == false && m_allocatedRanges.top().first <= completedFenceValue) {
		auto [freeFenceValue, freeRange] = m_allocatedRanges.top(); m_allocatedRanges.pop();
		// 向左融合
		if (freeRange.beg > 0) {
			auto lpart_end_iter = m_endToBeg.find(freeRange.beg - 1);
			if (lpart_end_iter != m_endToBeg.end()) {
				AllocateRange leftRange;
				leftRange.beg = lpart_end_iter->second;
				leftRange.end = lpart_end_iter->first;
				m_endToBeg.erase(lpart_end_iter);
				m_begToEnd.erase(leftRange.beg);
				auto sameSizeRanges = m_freeRanges.find(leftRange.AllocatedSize());
				for (auto& iter = sameSizeRanges->second.begin(); iter != sameSizeRanges->second.end(); ++iter) {
					if (leftRange == (*iter)) {
						sameSizeRanges->second.erase(iter);
						break;
					}
				}
				freeRange.beg = leftRange.beg;
			}
		}
		// 向右融合
		if (freeRange.end + 1 < Capacity()) {
			auto rpart_beg_iter = m_begToEnd.find(freeRange.end + 1);
			if (rpart_beg_iter != m_begToEnd.end()) {
				AllocateRange rightRange;
				rightRange.beg = rpart_beg_iter->first;
				rightRange.end = rpart_beg_iter->second;
				m_begToEnd.erase(rpart_beg_iter);
				m_endToBeg.erase(rightRange.end);
				auto sameSizeRanges = m_freeRanges.find(rightRange.AllocatedSize());
				for (auto& iter = sameSizeRanges->second.begin(); iter != sameSizeRanges->second.end(); ++iter) {
					if (rightRange == (*iter)) {
						sameSizeRanges->second.erase(iter);
						break;
					}
				}
				freeRange.end = rightRange.end;
			}
		}
		m_freeRanges[freeRange.AllocatedSize()].push_back(freeRange);
		m_begToEnd.insert({ freeRange.beg, freeRange.end });
		m_endToBeg.insert({ freeRange.end, freeRange.beg });
	}
}

END_NAME_SPACE


