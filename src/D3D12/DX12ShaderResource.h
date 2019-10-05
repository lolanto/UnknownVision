#pragma once
#include "DX12Config.h"
#include "../GPUResource/ShaderResource.h"

BEG_NAME_SPACE

class DX12ConstantBufferView : public ConstantBufferView {
public:
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_desc;
	ID3D12Resource* m_res;
};

class DX12ShaderResourceView : public ShaderResourceView {
public:
	D3D12_SHADER_RESOURCE_VIEW_DESC m_desc;
	ID3D12Resource* m_res;
};

class DX12UnorderAccessView : public UnorderAccessView {
public:
	D3D12_UNORDERED_ACCESS_VIEW_DESC m_desc;
	ID3D12Resource* m_res;
};

class DX12RenderTargetView : public RenderTargetView {
public:
	D3D12_RENDER_TARGET_VIEW_DESC m_desc;
	ID3D12Resource* m_res;
};

class DX12VertexBufferView : public VertexBufferView {
public:
	D3D12_VERTEX_BUFFER_VIEW m_view;
	ID3D12Resource* m_res;
};

class DX12IndexBufferView : public IndexBufferView {
public:
	D3D12_INDEX_BUFFER_VIEW m_view;
	ID3D12Resource* m_res;
};

class DX12DepthStencilView : public DepthStencilView {
public:
	D3D12_DEPTH_STENCIL_VIEW_DESC m_desc;
	ID3D12Resource* m_res;
};

END_NAME_SPACE
