#pragma once
#include "DX12Config.h"
#include <map>
#include <limits>
#include <functional>
#include <assert.h>

BEG_NAME_SPACE

extern UINT GDescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
constexpr size_t DEFAULT_NUMBER_OF_TRANSIENT_DESCRIPTORS = 128; /**< 默认的临时descirptor数量 */

class BasicDX12DescriptorHeap {
public:
	BasicDX12DescriptorHeap() { 
		clear();
	}
	virtual ~BasicDX12DescriptorHeap() = default;
	bool Initialize(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_DESC desc);
	virtual size_t RequestBlock(size_t size) { return 0; };
	virtual void Release(size_t beg, size_t offset) {};
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
		bool GPUVisible = true, /**< 指对CBV_SRV_UAV类型的heap起作用 */
		size_t numDescriptors = DEFAULT_NUMBER_OF_TRANSIENT_DESCRIPTORS) {
		m_head = 0;
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && GPUVisible) ?
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;
		return BasicDX12DescriptorHeap::Initialize(dev, desc);
	}
	virtual size_t RequestBlock(size_t size) { 
		assert(m_head + size < m_desc.NumDescriptors);
		size_t res = m_head;
		m_head += size;
		return res;
	};
	/** 临时资源一旦释放全都将释放，下面的两个参数无效 */
	virtual void Release(size_t head, size_t offset) {
		m_head = 0;
	};
	virtual void Reset() {
		m_head = 0;
		BasicDX12DescriptorHeap::Reset();
	};
private:
	size_t m_head;
};

END_NAME_SPACE
