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
	virtual bool RequestPermenent(RenderDevice* cmdUnit) override final;
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

class DX12Texture : public Texture {
	virtual RenderTargetView* GetRTVPtr() override final {
		
	}
private:
	/** 嘿！现在需要解决view应该怎么存储在资源中的问题，不是每一个资源都能够创建所有的VIEW，比如
	 * 普通的纹理就不需要作为RTV的VIEW，而目前设计的Resource基类提供所有的VIEW获取接口，这使得只定义
	 * 笼统的texture类从逻辑上变得臃肿——只读的，可以作为RTV的，都可以的，等等，这些特殊设计都需要被一个类
	 * 兼顾，考虑一下如何限制接口的数量的同时，满足这若干的特点 */
	std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> m_resource;
	std::array<uint32_t, RENDER_RESOURCE_VIEW_NUM> m_viewOffsets;
	std::vector<std::byte> m_views;
};


END_NAME_SPACE
