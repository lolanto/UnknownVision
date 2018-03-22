#include "Canvas.h"
#include "DXRenderer.h"
#include "InfoLog.h"

Canvas::Canvas(float width, float height, DXGI_FORMAT format, bool genMipMap, UINT arraySize)
	: width(width), height(height), 
	m_hasSetup(false), m_state(CS_UNKNOWN),
	format(format), hasMipmap(genMipMap), arraySize(arraySize),
	unorderAccess(false){}

bool Canvas::Setup(ID3D11Device* dev) {
	static const char* funcTag = "Canvas::Setup: ";
	if (m_hasSetup) return true;
	HRESULT hr;
	D3D11_TEXTURE2D_DESC texDesc;
	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	ZeroMemory(&RTVDesc, sizeof(RTVDesc));
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	// 当前未创建贴图
	if (!m_tex2d.Get()) {
		// Create texture
		texDesc.ArraySize = arraySize;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		if (unorderAccess) texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		texDesc.CPUAccessFlags = 0;
		// rgba 8bit per channel 0 ~ 255 treated as [0.0, 1.0]
		texDesc.Format = format;
		texDesc.Height = height;
		texDesc.Width = width;
		texDesc.MipLevels = hasMipmap ? 0 : 1;
		if (hasMipmap) texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;

		hr = dev->CreateTexture2D(&texDesc, NULL, m_tex2d.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			MLOG(LL, funcTag, LL, "create texture failed!");
			return false;
		}
	}

	// create render target
	RTVDesc.Format = texDesc.Format;
	if (arraySize > 1) {
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.ArraySize = arraySize;
		RTVDesc.Texture2DArray.FirstArraySlice = 0;
		RTVDesc.Texture2DArray.MipSlice = 0;
	}
	else {
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;
	}

	hr = dev->CreateRenderTargetView(m_tex2d.Get(), &RTVDesc, m_renderTarget.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LL, "create render target failed!");
		return false;
	}

	// create shader resource
	SRVDesc.Format = texDesc.Format;
	if (arraySize > 1) {
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		SRVDesc.Texture2DArray.ArraySize = arraySize;
		SRVDesc.Texture2DArray.FirstArraySlice = 0;
		SRVDesc.Texture2DArray.MipLevels = hasMipmap ? -1 : 1;
		SRVDesc.Texture2DArray.MostDetailedMip = 0;
	}
	else {
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = hasMipmap ? -1 : 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
	}

	hr = dev->CreateShaderResourceView(m_tex2d.Get(), NULL, m_shaderResource.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LL, "create shader resource view failed!");
		return false;
	}

	// create unorder access target if need
	if (unorderAccess) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		ZeroMemory(&UAVDesc, sizeof(UAVDesc));
		UAVDesc.Format = texDesc.Format;
		if (arraySize > 1) {
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			UAVDesc.Texture2DArray.ArraySize = arraySize;
			UAVDesc.Texture2DArray.FirstArraySlice = 0;
			UAVDesc.Texture2DArray.MipSlice = hasMipmap ? -1 : 1;
			UAVDesc.Texture2DArray.MipSlice = 0;
		}
		else {
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			UAVDesc.Texture2D.MipSlice = 0;
		}

		hr = dev->CreateUnorderedAccessView(m_tex2d.Get(), &UAVDesc, m_unorderAccessTarget.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			MLOG(LL, funcTag, LL, "create unorder access view failed!");
			return false;
		}
	}

	m_hasSetup = true;
	return true;
}

//////////////////////////////////////
// IRenderTarget
//////////////////////////////////////
ID3D11RenderTargetView* Canvas::GetRTV() { return m_renderTarget.Get(); }

void Canvas::UnbindRTV() {
	m_state = CS_UNKNOWN;
}

//////////////////////////////////////
// IShaderResource
//////////////////////////////////////

ID3D11ShaderResourceView** Canvas::GetSRV() { return m_shaderResource.GetAddressOf(); }

void Canvas::UnbindSRV() {
	m_state = CS_UNKNOWN;
}

///////////////////
// Unorder Access
///////////////////

ID3D11UnorderedAccessView** Canvas::GetUAV() { 
	if (unorderAccess) return m_unorderAccessTarget.GetAddressOf(); 
	return NULL;
}

void Canvas::UnbindUAV() {
	m_state = CS_UNKNOWN;
}

///////////////////
// IterateObject
///////////////////
void Canvas::IterFunc(ID3D11Device* dev, ID3D11DeviceContext* devCtx) {
	if (hasMipmap) devCtx->GenerateMips(m_shaderResource.Get());
}

/////////////////////////////////////
// public function
/////////////////////////////////////

/////////////////////////////////////
// private function
/////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   CanvasCubeMap   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CanvasCubeMap::CanvasCubeMap(float size, DXGI_FORMAT format)
	: size(size), format(format),
	m_state(CS_UNKNOWN), m_hasSetup(false) {}

bool CanvasCubeMap::Setup(ID3D11Device* dev) {
	static const char* funcTag = "CanvasCubeMap: ";
	if (m_hasSetup) return true;
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC texDesc;
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = format;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	// caution! cube map must have the same width and height;!
	texDesc.Width = size;
	texDesc.Height = size;
	// 6 elements for cube map
	texDesc.ArraySize = 6;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;

	hr = dev->CreateTexture2D(&texDesc, NULL, m_tex2d.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LE, "create cube map failed!");
		return false;
	}

	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 6;
	rtvDesc.Texture2DArray.FirstArraySlice = 0;
	rtvDesc.Texture2DArray.MipSlice = 0;
	hr = dev->CreateRenderTargetView(m_tex2d.Get(), &rtvDesc, m_renderTarget.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LE, "create cube map render target failed!");
		return false;
	}

	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.MostDetailedMip = 0;
	hr = dev->CreateShaderResourceView(m_tex2d.Get(), &srvDesc, m_shaderResource.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LE, "create cube map shader resource failed!");
		return false;
	}

	m_hasSetup = true;
	return true;
}

///////////////////
// Render Target
///////////////////

ID3D11RenderTargetView* CanvasCubeMap::GetRTV() { m_state = CS_RENDER_TARGET; return m_renderTarget.Get(); }
void CanvasCubeMap::UnbindRTV() { m_state = CS_UNKNOWN; }

///////////////////
// Shader Resource
///////////////////

ID3D11ShaderResourceView** CanvasCubeMap::GetSRV() { m_state = CS_SHADER_RESOURCE; return m_shaderResource.GetAddressOf(); }
void CanvasCubeMap::UnbindSRV() { m_state = CS_UNKNOWN; }
