#include "DX12BindingBoard.h"
#include "DX12RenderDevice.h"
#include "GPUResource/DX12GPUResource.h"
#include <assert.h>
BEG_NAME_SPACE

void DX12BindingBoard::Initialize(size_t capacity, DX12RenderDevice* pDevice, COMMAND_UNIT_TYPE type)
{
	assert(m_cpuHeap.IsReady() == false);
	m_pDevice = pDevice;
	m_type = type;
	m_alloc = AllocateRange::INVALID();
	m_lastFenceValue = 0;
	m_enableBinding = true;
	assert(m_cpuHeap.Initialize(pDevice->GetDevice(), pDevice->NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		capacity, false));
}

void DX12BindingBoard::BindingResource(size_t slotIdx, GPUResource* ptr, ShaderParameterType type)
{
	assert(m_enableBinding == true);
	auto handle = m_cpuHeap.GetCPUHandle(slotIdx);
	ID3D12Device* dev = m_pDevice->GetDevice();
	if (type == SHADER_PARAMETER_TYPE_BUFFER_R && ptr->Type() == GPU_RESOURCE_TYPE_BUFFER) {
		auto bufPtr = dynamic_cast<DX12Buffer*>(ptr);
		assert(bufPtr);
		auto&& view = bufPtr->GetConstantBufferView();
		dev->CreateConstantBufferView(&view, handle);
	}
	else if (type == SHADER_PARAMETER_TYPE_BUFFER_RW && ptr->Type() == GPU_RESOURCE_TYPE_BUFFER) {
		auto bufPtr = dynamic_cast<DX12Buffer*>(ptr);
		assert(bufPtr);
		auto&& view = bufPtr->GetUnorderedAccessView();
		/**TODO: 暂时不支持counter */
		dev->CreateUnorderedAccessView(
			reinterpret_cast<ID3D12Resource*>(bufPtr->GetResource()), nullptr,
			&view, handle);
	}
	else if (type == SHADER_PARAMETER_TYPE_TEXTURE_R) {
		if (ptr->Type() == GPU_RESOURCE_TYPE_TEXTURE2D) {
			auto texPtr = dynamic_cast<DX12Texture2D*>(ptr);
			assert(texPtr);
			auto&& view = texPtr->GetShaderResourceView();
			dev->CreateShaderResourceView(
				reinterpret_cast<ID3D12Resource*>(texPtr->GetResource()), &view, handle);
		}
		else {
			assert(false);
		}
	}
	else assert(false);
}

size_t DX12BindingBoard::Capacity() const {
	return m_cpuHeap.Capacity();
}

void DX12BindingBoard::Close()
{
	assert(m_enableBinding == true);
	UINT rangeSize = Capacity();
	m_alloc = m_pDevice->RequestDescriptorBlocks(rangeSize, m_type);
	m_pDevice->GetDevice()->CopyDescriptors(
		1, &m_alloc.cpuHandle, &rangeSize,
		1, &m_cpuHeap.GetCPUHandle(0), &rangeSize,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
	BindingBoard::Close();
}

void DX12BindingBoard::Reset() {
	assert(m_enableBinding == false);
	m_pDevice->ReleaseDescriptorBlocks(m_alloc, m_lastFenceValue, m_type);
	m_lastFenceValue = 0;
	m_alloc = AllocateRange::INVALID();
	BindingBoard::Reset();
}

END_NAME_SPACE
