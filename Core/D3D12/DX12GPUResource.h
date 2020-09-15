#pragma once
#include "../GraphicsInterface/GPUResource.h"
#include "DX12ResourceManager.h"
#include <d3d12.h>

BEG_NAME_SPACE
#ifdef API_TYPE == DX12

class DX12Buffer : public Buffer {
	friend class DX12RenderDevice;
public:
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const {
		D3D12_VERTEX_BUFFER_VIEW bufView;
		bufView.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
		bufView.SizeInBytes = MemFootprint();
		bufView.StrideInBytes = StrideInBytes();
		return bufView;
	}
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
	D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantBufferView() const {
		/** Note: 务必保证资源创建时符合CBV的要求：1. buffer的大小是256的倍数 */
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
		desc.SizeInBytes = m_pBuffer->GetDesc().Width; /**< MemFootprint仅代表使用的内存大小，CBV要求内存对齐到256，创建时务必保证满足这一要求 */
		return desc;
	}
	D3D12_UNORDERED_ACCESS_VIEW_DESC GetUnorderedAccessView() const {
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
		desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		switch (m_strideInBytes) {
		case 1:
			desc.Format = DXGI_FORMAT_R8_UINT; break;
		case 2:
			desc.Format = DXGI_FORMAT_R16_UINT; break;
		case 4:
			desc.Format = DXGI_FORMAT_R32_UINT; break;
		default:
			desc.Format = DXGI_FORMAT_UNKNOWN; break;
		}
		desc.Buffer.CounterOffsetInBytes = 0;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		desc.Buffer.NumElements = m_capacity;
		desc.Buffer.StructureByteStride = m_strideInBytes;
		return desc;
	}
public:
	DX12Buffer() : m_pBuffer(nullptr), m_pResMgr(nullptr) {}
	virtual ~DX12Buffer() = default;
	
	void* GetResource() override final {
		return m_pBuffer;
	}
	void SetName(const wchar_t* name) final {
		if (m_pBuffer)
			m_pBuffer->SetName(name);
	}
	bool Avaliable() const final {
		return m_pBuffer != nullptr && m_pResMgr != nullptr;
	}
	virtual void Release() final;
private:
	ID3D12Resource* m_pBuffer;
	DX12ResourceManager* m_pResMgr;
};

class DX12Texture2D : public Texture2D {
	friend class DX12RenderDevice;
public:
	D3D12_RENDER_TARGET_VIEW_DESC GetRenderTargetView(uint32_t mipSlice = 0);
	/** 返回DSV
	 * @param mipSlice 从哪个mipLevel 开始访问
	 * @param mipLevels 能够访问多少层mipLevel
	 * @param arr 作为纹理数组绑定，默认为非数组
	 * @param cubemap 是否作为cubemap绑定，默认为非cubemap
	 * @return 返回DSV描述
	 * @remark 当且仅当纹理本身的arrsize为6时，cubemap为true方才有效，否则异常 */
	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceView(uint32_t mipSlice = 0, uint32_t mipLevels = 1, size_t arr = 0, bool cubemap = false);
	/** 辅助函数 */
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRV_Single(uint32_t mipSlice = 0, uint32_t mipLevels = 1) { return GetShaderResourceView(mipSlice, mipLevels, 0, false); }
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRV_CUBE(uint32_t mipSlice = 0, uint32_t mipLevels = 1) { return GetShaderResourceView(mipSlice, mipLevels, 6, true); }
	D3D12_DEPTH_STENCIL_VIEW_DESC GetDepthStencilView(uint32_t mipSlice = 0);
public:
	DX12Texture2D() : m_pTexture(nullptr), m_pResMgr(nullptr) {}
	virtual ~DX12Texture2D() = default;

	void* GetResource() override final { return m_pTexture; }

	void SetName(const wchar_t* name) final {
		if (m_pTexture)
			m_pTexture->SetName(name);
	}

	bool Avaliable() const final {
		return m_pTexture != nullptr && m_pResMgr != nullptr;
	}

	virtual void Release() final;
private:
	ID3D12Resource* m_pTexture;
	DX12ResourceManager* m_pResMgr;
};

#endif // API_TYPE == DX12
END_NAME_SPACE
