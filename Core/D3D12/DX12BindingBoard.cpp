#include "DX12BindingBoard.h"
#include "DX12RenderDevice.h"
#include "DX12GPUResource.h"
#include <cassert>
#include "../../Utility/InfoLog/InfoLog.h"
BEG_NAME_SPACE

void DX12BindingBoard::Initialize(size_t capacity, DX12RenderDevice* pDevice, COMMAND_UNIT_TYPE type)
{
	if (m_cpuHeap.IsReady() == true) {
		LOG_ERROR("cpu heap has been initialized! you may do some error operation!");
		abort();
	}
	m_pDevice = pDevice;
	m_type = type;
	m_alloc = AllocateRange::INVALID();
	m_lastFenceValue = 0;
	m_enableBinding = true;
	if (m_cpuHeap.Initialize(pDevice->GetDevice(), pDevice->NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		capacity, false) == false) {
		LOG_ERROR("Initialized CPU heap failed!");
		abort();
	}
}

void DX12BindingBoard::BindingResource(size_t slotIdx, GPUResource* ptr, ShaderParameterType type, ShaderParameterFlag flag1, int flag2)
{
	if (m_enableBinding == false) {
		LOG_ERROR("Can not binding anymore until you call reset!");
		abort();
	}
	auto handle = m_cpuHeap.GetCPUHandle(slotIdx);
	ID3D12Device* dev = m_pDevice->GetDevice();
	if (type == SHADER_PARAMETER_TYPE_BUFFER_R && ptr->Type() == GPU_RESOURCE_TYPE_BUFFER) {
		auto bufPtr = dynamic_cast<DX12Buffer*>(ptr);
		if (bufPtr == nullptr) {
			LOG_ERROR("Parameter type and GPU Resource doesn't match!");
			abort();
		}
		auto&& view = bufPtr->GetConstantBufferView();
		dev->CreateConstantBufferView(&view, handle);
	}
	else if (type == SHADER_PARAMETER_TYPE_BUFFER_RW && ptr->Type() == GPU_RESOURCE_TYPE_BUFFER) {
		auto bufPtr = dynamic_cast<DX12Buffer*>(ptr);
		if (bufPtr == nullptr) {
			LOG_ERROR("Parameter type and GPU Resource doesn't match!");
			abort();
		}
		auto&& view = bufPtr->GetUnorderedAccessView();
		/**TODO: 暂时不支持counter */
		dev->CreateUnorderedAccessView(
			reinterpret_cast<ID3D12Resource*>(bufPtr->GetResource()), nullptr,
			&view, handle);
	}
	else if (type == SHADER_PARAMETER_TYPE_TEXTURE_R) {
		if (ptr->Type() == GPU_RESOURCE_TYPE_TEXTURE2D) {
			auto texPtr = dynamic_cast<DX12Texture2D*>(ptr);
			if (texPtr == nullptr) {
				LOG_ERROR("Parameter type and GPU Resource doesn't match!");
				abort();
			}
			D3D12_SHADER_RESOURCE_VIEW_DESC view;
			switch (flag1) {
			case SHADER_PARAMETER_FLAG_NONE:
				view = texPtr->GetShaderResourceView();
				break;
			case SHADER_PARAMETER_FLAG_CUBE:
				view = texPtr->GetSRV_CUBE();
				break;
			default:
				abort();
			}
			dev->CreateShaderResourceView(
				reinterpret_cast<ID3D12Resource*>(texPtr->GetResource()), &view, handle);
		}
		else {
			LOG_WARN("Texture Type doesn't support!");
		}
	}
	else {
		LOG_WARN("Parameter type doesn't support!");
	}
}

size_t DX12BindingBoard::Capacity() const {
	return m_cpuHeap.Capacity();
}

void DX12BindingBoard::Close()
{
	if (m_enableBinding == false) {
		LOG_ERROR("Can not close it again until you call Reset!");
		abort();
	}
	UINT rangeSize = Capacity();
	m_alloc = m_pDevice->RequestDescriptorBlocks(rangeSize, m_type);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_cpuHeap.GetCPUHandle(0);
	m_pDevice->GetDevice()->CopyDescriptors(
		1, &m_alloc.cpuHandle, &rangeSize,
		1, &cpuHandle, &rangeSize,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
	BindingBoard::Close();
}

void DX12BindingBoard::Reset() {
	if (m_enableBinding == true) {
		LOG_ERROR("Can not reset it again until you call Close");
		abort();
	}
	m_pDevice->ReleaseDescriptorBlocks(m_alloc, m_lastFenceValue, m_type);
	m_lastFenceValue = 0;
	m_alloc = AllocateRange::INVALID();
	BindingBoard::Reset();
}

END_NAME_SPACE
