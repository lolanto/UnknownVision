#pragma once
#include "DX12Config.h"
#include <map>
#include <limits>
#include <functional>
#include <assert.h>

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
	bool Initialize(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_TYPE type,
		size_t numberOfDescriptors, bool bShaderVisibility = false);
	/** 重置整个Descriptor Heap，原有的内容将被释放 */
	virtual void Reset() { clear(); };
public:
	const ID3D12DescriptorHeap* GetHeap() const { return m_descHeap.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(size_t beg) const { 
		assert(beg < m_desc.NumDescriptors);
		return { m_cpu_beg.ptr + beg * GDescriptorHandleIncrementSize[m_desc.Flags] };
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t beg) const {
		assert(beg < m_desc.NumDescriptors);
		return { m_gpu_beg.ptr + beg * GDescriptorHandleIncrementSize[m_desc.Flags] };
	}
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

/** 临时Descriptor Heap，线性的分配空间，而且不进行回收
 * 用来临时存储CommandList用的Descriptor heap */
class TransientDX12DescriptorHeap : public BasicDX12DescriptorHeap {
public:
	bool Initialize(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_TYPE type,
		size_t numDescriptors = DEFAULT_NUMBER_OF_TRANSIENT_DESCRIPTORS,
		bool bShaderVisibility = true /**< 指对CBV_SRV_UAV类型的heap起作用 */) {
		m_head = 0;
		return BasicDX12DescriptorHeap::Initialize(dev, type, numDescriptors, bShaderVisibility);
	}
	virtual size_t RequestBlock(size_t size) { 
		assert(m_head + size < m_desc.NumDescriptors);
		size_t res = m_head;
		m_head += size;
		return res;
	};
	/** 临时资源一旦释放全都将释放 */
	void Release() {
		m_head = 0;
	};
	virtual void Reset() {
		m_head = 0;
		BasicDX12DescriptorHeap::Reset();
	};
private:
	size_t m_head;
};

/** 离散静态的Descriptor heap，用来存储长期资源的descriptor handle
 * 不能用来存储shader visible的descriptor handle
 * 管理方式以尽可能重用为主
* TODO: 目前处于开发阶段，暂时以线性方式进行分配，但需要固定好接口 */
class DiscretePermanentDX12DescriptorHeap : public BasicDX12DescriptorHeap {
public:
	bool Initialize(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_TYPE type,
		size_t numberOfDescriptors = DEFAULT_NUMBER_OF_PERMENENT_DESCRIPTORS) {
		m_capacity = numberOfDescriptors;
		m_nextHead = 0;
		return BasicDX12DescriptorHeap::Initialize(dev, type, numberOfDescriptors, false);
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


/** 该heap以环的形式进行管理
 * 用来存放动态的GPU可见的descriptors
 * 局部管理表示一个区域由一系列连续的分配构成，每次释放以区域为单位进行*/
class LocalDynamicDX12DescriptorHeap : public BasicDX12DescriptorHeap {
private:
	struct AllocatedBlockInfo {
		size_t head;
		size_t size;
	};
public:
	bool Initialize(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_TYPE type,
		size_t numberOfDescriptors = DEFAULT_NUMBER_OF_PERMENENT_DESCRIPTORS) {
		m_capacity = numberOfDescriptors;
		m_bBeginAllocate = false;
		m_begin = m_end = m_nextHead = 0;
		m_hashDescriptors.clear();
		m_allocatedBlocks.clear();
		return BasicDX12DescriptorHeap::Initialize(dev, type, numberOfDescriptors, true);
	}
	/** 表示区域分配开始
	 * @return 返回分区起始偏移 */
	size_t BeginAllocation();
	/** 当且仅当新区域开始分配时，才允许请求连续的block片段
	 * @param size 需要请求的descriptor数量
	 * @param pResources 用来进行hash的资源地址 
	 * @remark 务必确保resource的view能够存储于相同的descriptor heap中 */
	size_t RequestBlock(size_t size, ID3D12Resource** pResources);
	/** 区域分配结束 */
	void EndAllocation();
	/** 释放一个分区
	 * @param head 分区的起始位置 */
	void Release(size_t head);
private:
	bool m_bBeginAllocate;
	size_t m_capacity;
	size_t m_begin; /**< 当前分区起始位置 */
	size_t m_nextHead; /**< 下一个分配块的起始位置 */
	size_t m_end; /**< 上一个分区的结束位置 */
	std::map<uint64_t, D3D12_GPU_DESCRIPTOR_HANDLE> m_hashDescriptors; /**< 当前片段已经hash过的 */
	std::vector<AllocatedBlockInfo> m_allocatedBlocks; /**< 已经分配过的块 */
};

END_NAME_SPACE
