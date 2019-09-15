#pragma once
#include "../Resource/RenderResource.h"
#include "DX12ShaderResource.h"
#include "DX12ResourceManager.h"
#include "DX12RenderBasic.h"
#include "DX12Helpers.h"

BEG_NAME_SPACE

extern class DX12RenderDevice GDevice;

class DX12Buffer : public Buffer {
public:
	virtual bool Initialize(ResourceStatus status) override final {
		m_resource = GDevice.ResourceManager().RequestBuffer(m_size, ResourceStatusToResourceFlag(status),
			ResourceStatusToHeapType(status));
		if (m_resource.first == nullptr) return false;
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = m_resource.first->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_size;
		return true;
	}
	virtual ConstantBufferView* GetCBVPtr() override final {
		return &m_cbv;
	}
public:
	ID3D12Resource* GetResource() { return m_resource.first; }
	D3D12_RESOURCE_STATES GetResourceState() const { return m_resource.second; }
	D3D12_RESOURCE_STATES& GetResourceState() { return m_resource.second; }
private:
	std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> m_resource;
	DX12ConstantBufferView m_cbv;
};

END_NAME_SPACE
