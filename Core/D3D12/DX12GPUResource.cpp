#include "DX12GPUResource.h"
#include "DX12Helpers.h"

BEG_NAME_SPACE

D3D12_RENDER_TARGET_VIEW_DESC DX12Texture2D::GetRenderTargetView(uint32_t mipSlice) {
	if (m_status.canBeRenderTarget() == false) {
		LOG_ERROR("this resource can not be used as render target!");
		abort();
	}
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = ElementFormatToDXGIFormat(m_format);
	rtvDesc.Texture2D.MipSlice = mipSlice;
	rtvDesc.Texture2D.PlaneSlice = 0; /**< TODO: 暂不支持plane slice */
	return rtvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC DX12Texture2D::GetShaderResourceView(uint32_t mipSlice, uint32_t mipLevels, size_t arr, bool cubemap)
{
	if (m_status.canBeShaderResource() == false) {
		LOG_ERROR("this resource can not be used as shader resource!");
		abort();
	}
	if (mipSlice >= m_mipLevels || mipSlice + mipLevels > m_mipLevels) {
		LOG_ERROR("Invalid mip settings! Request Slice: %d, Request Levels: %d, Actual Miplevels: %d", mipSlice, mipLevels, m_mipLevels);
		abort();
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC desc;
	desc.Format = ElementFormatToDXGIFormat(m_format);
	// 该值用来限制shader能够访问的通道，默认全部通道都可以访问
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (arr == 0) {
		// 存粹的一张2D纹理
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = mipLevels;
		desc.Texture2D.MostDetailedMip = mipSlice;
		desc.Texture2D.PlaneSlice = 0; /**< TODO: 暂时不支持plane slice */
		desc.Texture2D.ResourceMinLODClamp = 0.0f; /**< TODO: 暂时不支持clamp */
	}
	else {
		if (arr > m_arrSize) {
			LOG_ERROR("Invalid array size! Request Size: %d, Actual Size: %d", arr, m_arrSize);
			abort();
		}
		if (arr == 6 && cubemap) {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MipLevels = mipLevels;
			desc.TextureCube.MostDetailedMip = mipSlice;
			desc.TextureCube.ResourceMinLODClamp = 0.0f; /**< TODO: 暂时不支持clamp */
		}
		else {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.ArraySize = arr;
			desc.Texture2DArray.FirstArraySlice = 0; /**< TODO: 暂时不支持偏移访问数组 */
			desc.Texture2DArray.MipLevels = mipLevels;
			desc.Texture2DArray.MostDetailedMip = mipSlice;
			desc.Texture2DArray.PlaneSlice = 0; /**< TODO: 暂时不支持plane slice */
			desc.Texture2DArray.ResourceMinLODClamp = 0.0f; /**< TODO: 暂时不支持clamp */
		}
	}
	return desc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC DX12Texture2D::GetDepthStencilView(uint32_t mipSlice)
{
	if (m_status.canBeDepthStencil() == false) {
		LOG_ERROR("this resource can not be used as depth/stencil buffer");
		abort();
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Format = ElementFormatToDXGIFormat(m_format);
	desc.Texture2D.MipSlice = mipSlice;
	desc.Flags = D3D12_DSV_FLAG_NONE;
	return desc;
}

void DX12Texture2D::Release() {
	if (Avaliable()) {
		if (m_pResMgr->ReleaseResource(m_pTexture) == false) {
			LOG_WARN("Release resource failed! Maybe there is some error occur!");
		}
		m_pTexture = nullptr;
		m_pResMgr = nullptr;
	}
}

D3D12_INDEX_BUFFER_VIEW DX12Buffer::GetIndexBufferView() const {
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
		LOG_ERROR("Invalid index value size!");
		abort();
	}
	idxView.SizeInBytes = MemFootprint();
	return idxView;
}

void DX12Buffer::Release() {
	if (Avaliable()) {
		if (m_pResMgr->ReleaseResource(m_pBuffer) == false) {
			LOG_WARN("Release resource failed! Maybe there is some error occur!");
		}
		m_pBuffer = nullptr;
		m_pResMgr = nullptr;
	}
}

END_NAME_SPACE
