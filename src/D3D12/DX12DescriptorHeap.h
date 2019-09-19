#pragma once
#include "DX12Config.h"
#include <map>
#include <limits>
#include <functional>
#include <assert.h>

BEG_NAME_SPACE

extern UINT DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];



class BasicDX12DescriptorHeap {
public:
	struct HeapBlock {
		size_t beg, end;
	};
	static bool Copy(const BasicDX12DescriptorHeap& src, size_t srcBeg,
		BasicDX12DescriptorHeap& dest, size_t destBeg,
		size_t size);
public:
	BasicDX12DescriptorHeap() { 
		m_gpu_beg.ptr = std::numeric_limits<decltype(m_gpu_beg.ptr)>::max();
		m_cpu_beg.ptr = std::numeric_limits<decltype(m_cpu_beg.ptr)>::max();
		memset(&m_desc, 0, sizeof(m_desc));
	}
	virtual ~BasicDX12DescriptorHeap() = default;
	virtual bool Initialize(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_DESC desc);
	bool IsShaderVisible() const { return m_gpu_beg.ptr != std::numeric_limits<decltype(m_gpu_beg.ptr)>::max(); }
public:
	const ID3D12DescriptorHeap* GetHeap() const { return m_descHeap.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(size_t beg) const { 
		assert(beg < m_desc.NumDescriptors);
		return { m_cpu_beg.ptr + beg * DescriptorHandleIncrementSize[m_desc.Flags] };
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t beg) const {
		assert(beg < m_desc.NumDescriptors);
		return { m_gpu_beg.ptr + beg * DescriptorHandleIncrementSize[m_desc.Flags] };
	}
	virtual HeapBlock RequestBlock(size_t size) { return { 0, 0 }; };
	virtual void Release(HeapBlock block) {};
	virtual void Reset() {};

	std::function<void()> Register_Reset_Event() {
		decltype(this) heapPtr = this;
		return [heapPtr]() { heapPtr->Reset(); };
	}

	std::function<void()> Register_Release_Event(HeapBlock block) {
		decltype(this) heapPtr = this;
		return [heapPtr, block]() { heapPtr->Release(block); };
	}

protected:
	SmartPTR<ID3D12DescriptorHeap> m_descHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_desc;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_beg;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_beg;
};

/** 临时Descriptor Heap，每次CommandList结束都可能会被清空，采用栈式管理 */
class TransientDX12DescriptorHeap : public BasicDX12DescriptorHeap {
public:
	bool Initialize(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_DESC desc) override final {
		m_head = 0;
		return BasicDX12DescriptorHeap::Initialize(dev, desc);
	}
	virtual HeapBlock RequestBlock(size_t size) { 
		assert(m_head + size < m_desc.NumDescriptors);
		HeapBlock res;
		res.beg = m_head;
		res.end = (m_head += size);
		return res;
	};
	/** 临时资源不进行释放 */
	virtual void Release(HeapBlock block) {};
	virtual void Reset() {
		m_head = 0;
	};
private:
	size_t m_head;
};

END_NAME_SPACE
