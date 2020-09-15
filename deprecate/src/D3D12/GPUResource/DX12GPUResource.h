#include "../../GPUResource/GPUResource.h"
#include "../DX12ResourceManager.h"
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
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const {
		D3D12_INDEX_BUFFER_VIEW idxView;
		idxView.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
		switch (m_strideInBytes) {
		case 1:
			idxView.Format = DXGI_FORMAT_R8_UINT; break;
		case 2:
			idxView.Format = DXGI_FORMAT_R16_UINT; break;
		case 4:
			idxView.Format = DXGI_FORMAT_R32_UINT; break;
		default:
			assert(false);
		}
		idxView.SizeInBytes = MemFootprint();
		return idxView;	
	}
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
	virtual void Release() final {
		if (Avaliable()) {
			assert(m_pResMgr->ReleaseResource(m_pBuffer));
			m_pBuffer = nullptr;
			m_pResMgr = nullptr;
		}
	}
private:
	ID3D12Resource* m_pBuffer;
	DX12ResourceManager* m_pResMgr;
};

class DX12Texture2D : public Texture2D {
	friend class DX12RenderDevice;
public:
	D3D12_RENDER_TARGET_VIEW_DESC GetRenderTargetView(uint32_t mipSlice = 0) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = ElementFormatToDXGIFormat(m_format);
		rtvDesc.Texture2D.MipSlice = mipSlice;
		rtvDesc.Texture2D.PlaneSlice = 0; /**< TODO: 暂不支持plane slice */
		return rtvDesc;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceView(uint32_t mipSlice = 0, uint32_t mipLevels = 1) {
		D3D12_SHADER_RESOURCE_VIEW_DESC desc;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Format = ElementFormatToDXGIFormat(m_format);
		desc.Texture2D.MipLevels = mipLevels;
		desc.Texture2D.MostDetailedMip = mipSlice;
		desc.Texture2D.PlaneSlice = 0; /**< TODO: 暂时不支持plane slice */
		desc.Texture2D.ResourceMinLODClamp = 0.0f;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		return desc;
	}
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

	virtual void Release() final {
		if (Avaliable()) {
			assert(m_pResMgr->ReleaseResource(m_pTexture));
			m_pTexture = nullptr;
			m_pResMgr = nullptr;
		}
	}
private:
	ID3D12Resource* m_pTexture;
	DX12ResourceManager* m_pResMgr;
};

#endif // API_TYPE == DX12
END_NAME_SPACE
