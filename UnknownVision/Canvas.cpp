#include "Canvas.h"
#include "DXRenderer.h"
#include "InfoLog.h"
#include <assert.h>

Canvas::Canvas(float width, float height, DXGI_FORMAT format)
	: width(width), height(height), 
	format(format), 
	m_isUnorderAccess(false), m_simultaneouslyRW(false),
	m_hasMipmap(false), m_arraySize(1){
}

void Canvas::preSetDesc() {
	ZeroMemory(&m_texDesc, sizeof(m_texDesc));
	ZeroMemory(&m_rtvDesc, sizeof(m_rtvDesc));
	ZeroMemory(&m_srvDesc, sizeof(m_srvDesc));

	if (m_isUnorderAccess && m_simultaneouslyRW) assert(formatCheckForUARes());

	// Create texture
	m_texDesc.ArraySize = m_arraySize;
	m_texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (m_isUnorderAccess) m_texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	m_texDesc.CPUAccessFlags = 0;
	m_texDesc.Format = format;
	m_texDesc.Height = height;
	m_texDesc.Width = width;
	m_texDesc.MipLevels = m_hasMipmap ? 0 : 1;
	if (m_hasMipmap) m_texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	m_texDesc.SampleDesc.Count = 1;
	m_texDesc.SampleDesc.Quality = 0;
	m_texDesc.Usage = D3D11_USAGE_DEFAULT;

	// create render target
	m_rtvDesc.Format = m_texDesc.Format;
	if (m_arraySize > 1) {
		m_rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		m_rtvDesc.Texture2DArray.ArraySize = m_arraySize;
		m_rtvDesc.Texture2DArray.FirstArraySlice = 0;
		m_rtvDesc.Texture2DArray.MipSlice = 0;
	}
	else {
		m_rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		m_rtvDesc.Texture2D.MipSlice = 0;
	}

	// create shader resource
	m_srvDesc.Format = m_texDesc.Format;
	if (m_arraySize > 1) {
		m_srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		m_srvDesc.Texture2DArray.ArraySize = m_arraySize;
		m_srvDesc.Texture2DArray.FirstArraySlice = 0;
		m_srvDesc.Texture2DArray.MipLevels = m_hasMipmap ? -1 : 1;
		m_srvDesc.Texture2DArray.MostDetailedMip = 0;
	}
	else {
		m_srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		m_srvDesc.Texture2D.MipLevels = m_hasMipmap ? -1 : 1;
		m_srvDesc.Texture2D.MostDetailedMip = 0;
	}

	// create unorder access target if need
	if (m_isUnorderAccess) {
		ZeroMemory(&m_uavDesc, sizeof(m_uavDesc));
		m_uavDesc.Format = m_texDesc.Format;
		if (m_arraySize > 1) {
			m_uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			m_uavDesc.Texture2DArray.ArraySize = m_arraySize;
			m_uavDesc.Texture2DArray.FirstArraySlice = 0;
			m_uavDesc.Texture2DArray.MipSlice = m_hasMipmap ? -1 : 1;
			m_uavDesc.Texture2DArray.MipSlice = 0;
		}
		else {
			m_uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			m_uavDesc.Texture2D.MipSlice = 0;
		}
	}
}

bool Canvas::Setup(ID3D11Device* dev) {
	preSetDesc();
	HRESULT hr;
	hr = dev->CreateTexture2D(&m_texDesc, NULL, m_tex2d.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "create texture failed!");
		return false;
	}


	hr = dev->CreateRenderTargetView(m_tex2d.Get(), &m_rtvDesc, m_rtv.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "create render target failed!");
		return false;
	}

	hr = dev->CreateShaderResourceView(m_tex2d.Get(), &m_srvDesc, m_srv_tex.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "create shader resource view failed!");
		return false;
	}

	// create unorder access target if need
	if (m_isUnorderAccess) {
		hr = dev->CreateUnorderedAccessView(m_tex2d.Get(), &m_uavDesc, m_uav.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			MLOG(LL, __FUNCTION__, LL, "create unorder access view failed!");
			return false;
		}
	}

	return true;
}

/////////////////////////////////////
// public function
/////////////////////////////////////
void Canvas::GenMipMap(ID3D11DeviceContext* devCtx) {
	if (m_hasMipmap) ITexture::GenMipMap(devCtx);
}
void Canvas::GenArray(UINT size) { m_arraySize = size; }
void Canvas::SetMipmap(bool v) { m_hasMipmap = v; }
void Canvas::SetUARes(bool ua, bool simRW) { m_isUnorderAccess = ua; m_simultaneouslyRW = simRW; }

ID3D11Texture2D * Canvas::GetTex()
{
	return m_tex2d.Get();
}


/////////////////////////////////////
// private function
/////////////////////////////////////
bool Canvas::formatCheckForUARes() {
	switch (format) {
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R32_UINT:
		return true;
	default:
		return false;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   CanvasCubeMap   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CanvasCubeMap::CanvasCubeMap(float size, DXGI_FORMAT format)
	: Canvas(size, size, format) {
	GenArray();
}

bool CanvasCubeMap::Setup(ID3D11Device* dev) {
	preSetDesc();
	return Canvas::Setup(dev);
}

void CanvasCubeMap::preSetDesc()
{
	Canvas::preSetDesc();
	m_texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	m_srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	m_srvDesc.TextureCube.MipLevels = 1;
	m_srvDesc.TextureCube.MostDetailedMip = 0;
}
