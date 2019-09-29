#pragma once
#include "DX12Config.h"
#include "../Resource/ShaderResource.h"

BEG_NAME_SPACE

class DX12ConstantBufferView : public ConstantBufferView {
	friend class DX12Buffer;
	friend class DX12Texture;
private:
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_desc;
};

class DX12ShaderResourceView : public ShaderResourceView {
	friend class DX12Buffer;
	friend class DX12Texture;
private:
	D3D12_SHADER_RESOURCE_VIEW_DESC m_desc;
};

class DX12UnorderAccessView : public UnorderAccessView {
	friend class DX12Buffer;
	friend class DX12Texture;
private:
	D3D12_UNORDERED_ACCESS_VIEW_DESC m_desc;
};

class DX12RenderTargetView : public RenderTargetView {
	friend class DX12Buffer;
	friend class DX12Texture;
private:
	D3D12_RENDER_TARGET_VIEW_DESC m_desc;
};

END_NAME_SPACE
