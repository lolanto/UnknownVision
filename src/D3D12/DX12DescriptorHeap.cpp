#include "DX12DescriptorHeap.h"
#include <assert.h>
#include <limits>
#include <D3D12MemAlloc.h>
BEG_NAME_SPACE

bool BasicDX12DescriptorHeap::Initialize(ID3D12Device * dev, D3D12_DESCRIPTOR_HEAP_DESC desc)
{
	
	assert(FAILED(dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descHeap))));
	m_desc = desc;
	m_cpu_beg = m_descHeap->GetCPUDescriptorHandleForHeapStart();
	if (desc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		m_gpu_beg = m_descHeap->GetGPUDescriptorHandleForHeapStart();
	else
		m_gpu_beg.ptr = std::numeric_limits<decltype(m_gpu_beg.ptr)>::max();
	return true;
}

END_NAME_SPACE

