#pragma once
#include "DX12Config.h"
#include <map>
#include <unordered_map>
#include <limits>
#include <functional>
#include <queue>

BEG_NAME_SPACE

extern UINT GDescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
constexpr size_t DEFAULT_NUMBER_OF_TRANSIENT_DESCRIPTORS = 128; /**< 默认的临时descirptor数量 */
constexpr size_t DEFAULT_NUMBER_OF_PERMENENT_DESCRIPTORS = 128; /**< 长期存在的descriptor数量 */

class BasicDX12DescriptorHeap {
public:
	BasicDX12DescriptorHeap() { 
		clear();
	}
	virtual ~BasicDX12DescriptorHeap() = default;
	/** 用来封装Descriptor Heap的构造过程 */
	bool Initialize(ID3D12Device* dev, size_t nodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type,
		size_t numberOfDescriptors, bool bShaderVisibility = false);
	/** 重置整个Descriptor Heap，原有的内容将被释放 */
	virtual void Reset() { clear(); };
public:
	ID3D12DescriptorHeap* GetHeap() const { return m_descHeap.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(size_t beg) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t beg) const;
	/** 当前heap能容纳多少descriptor */
	size_t Capacity() const { return m_desc.NumDescriptors; }
	bool IsShaderVisible() const { return m_gpu_beg.ptr != std::numeric_limits<decltype(m_gpu_beg.ptr)>::max(); }
	bool IsReady() const { return m_descHeap != nullptr; }
protected:
	void clear() {
		m_descHeap.Reset(); 
		m_gpu_beg.ptr = std::numeric_limits<decltype(m_gpu_beg.ptr)>::max();
		m_cpu_beg.ptr = std::numeric_limits<decltype(m_cpu_beg.ptr)>::max();
		memset(&m_desc, 0, sizeof(m_desc));
	}
	SmartPTR<ID3D12DescriptorHeap> m_descHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_desc;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_beg;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_beg;
};

/** 离散静态的Descriptor heap，用来存储长期资源的descriptor handle
 * 不能用来存储shader visible的descriptor handle
 * 管理方式以尽可能重用为主
* TODO: 目前处于开发阶段，暂时以线性方式进行分配，但需要固定好接口 */
class DiscretePermanentDX12DescriptorHeap : public BasicDX12DescriptorHeap {
public:
	bool Initialize(ID3D12Device* dev, size_t nodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type,
		size_t numberOfDescriptors = DEFAULT_NUMBER_OF_PERMENENT_DESCRIPTORS) {
		m_capacity = numberOfDescriptors;
		m_nextHead = 0;
		return BasicDX12DescriptorHeap::Initialize(dev, nodeMask, type, numberOfDescriptors, false);
	}
	/** 请求一个block */
	D3D12_CPU_DESCRIPTOR_HANDLE RequestBlock();
	/** 释放申请的块
	 * @param head 申请的块在heap上的起始偏移 */
	void Release(D3D12_CPU_DESCRIPTOR_HANDLE block);
private:
	size_t m_capacity; /**< 总容量 */
	size_t m_nextHead; /**< 下一个要分配的块开头 */
};


/** 用来存放动态的GPU可见的descriptors
 * 局部管理表示一个区域由一系列连续的分配构成，每次释放以区域为单位进行*/
class LocalDynamicDX12DescriptorHeap : public BasicDX12DescriptorHeap {
private:
	using FenceValueToAllocateRange = std::pair<size_t, AllocateRange>;
	using CUSTOM_CMP = bool(*)(const FenceValueToAllocateRange & a, const FenceValueToAllocateRange & b);
	static bool custom_cmp(const FenceValueToAllocateRange& a, const FenceValueToAllocateRange& b) { return a.first > b.first; }
public:
	LocalDynamicDX12DescriptorHeap() : m_allocatedRanges(custom_cmp) {}
	virtual ~LocalDynamicDX12DescriptorHeap() = default;
	bool Initialize(ID3D12Device* dev,size_t nodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type,
		size_t numberOfDescriptors = DEFAULT_NUMBER_OF_PERMENENT_DESCRIPTORS) {
		//m_allocatedRanges.swap(decltype(m_allocatedRanges)(custom_cmp));
		m_allocatedRanges = decltype(m_allocatedRanges)(custom_cmp);
		AllocateRange range;
		range.beg = 0; range.end = numberOfDescriptors - 1;
		m_freeRanges.clear();
		m_begToEnd.clear(); m_endToBeg.clear();
		m_freeRanges.insert({ range.AllocatedSize(), { range } });
		m_begToEnd.insert({ range.beg, range.end });
		m_endToBeg.insert({ range.end, range.beg });
		return BasicDX12DescriptorHeap::Initialize(dev, nodeMask, type, numberOfDescriptors, true);
	}
	/** 请求一段连续的heap */
	AllocateRange RequestBlock(size_t capacity, size_t completedFenceValue);
	void ReleaseBlock(AllocateRange range, size_t lastFenceValue);
private:
	void updateFreeRanges(size_t completedFenceValue);
private:
	std::priority_queue<FenceValueToAllocateRange,
		std::vector<FenceValueToAllocateRange>,
		CUSTOM_CMP> m_allocatedRanges;
	std::map<size_t, std::list<AllocateRange>> m_freeRanges;
	std::unordered_map<size_t, size_t> m_begToEnd;
	std::unordered_map<size_t, size_t> m_endToBeg;
};

END_NAME_SPACE
