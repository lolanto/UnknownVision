#pragma once
#include "../Resource/RenderResource.h"
#include "DX12ShaderResource.h"
#include "DX12ResourceManager.h"
#include "DX12RenderBasic.h"
#include "DX12Helpers.h"

BEG_NAME_SPACE

class DX12Buffer : public Buffer {
public:
	/** 请求固定的资源，资源的释放需要手动控制 */
	virtual bool RequestPermenent(RenderDevice* cmdUnit, ResourceStatus status) override final ;
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
