#include <wincodec.h>
#include <memory>
#include "Texture.h"
#include "InfoLog.h"

using DirectX::Image;
using DirectX::ScratchImage;

void editDescritionStructure(const Image* img,
	D3D11_TEXTURE2D_DESC& tex2dDesc,
	D3D11_SUBRESOURCE_DATA& tex2dSubData,
	D3D11_SHADER_RESOURCE_VIEW_DESC& tex2dSRVDesc) {
	ZeroMemory(&tex2dDesc, sizeof(tex2dDesc));
	ZeroMemory(&tex2dSubData, sizeof(tex2dSubData));
	// set texture2d desc
	tex2dDesc.ArraySize = 1;
	tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.Format = img->format;
	tex2dDesc.Height = img->height;
	tex2dDesc.Width = img->width;
	// 不使用mipmap
	tex2dDesc.MipLevels = 1;
	tex2dDesc.MiscFlags = 0;
	// 不使用多重采样抗锯齿
	tex2dDesc.SampleDesc.Count = 1;
	tex2dDesc.SampleDesc.Quality = 0;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
	// set texture2d subresource data
	tex2dSubData.pSysMem = img->pixels;
	tex2dSubData.SysMemPitch = img->rowPitch;
	tex2dSubData.SysMemSlicePitch = img->slicePitch;

	// create shader resource view
	tex2dSRVDesc.Format = img->format;
	tex2dSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	tex2dSRVDesc.Texture2D.MipLevels = -1;
	tex2dSRVDesc.Texture2D.MostDetailedMip = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////   CommonTexture   //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CommonTexture::CommonTexture(const wchar_t* file) : m_source(file) {}

/////////////////////////
// public Function
/////////////////////////

bool CommonTexture::Setup(ID3D11Device* dev) {
	const char* funcTag = "CommonTexture::Setup: ";
	// load image file!
	auto image = std::make_shared<ScratchImage>();
	if (!TextureFactory::GetInstance().LoadCommonTextureFromFile(m_source, image)) {
		MLOG(LL, funcTag, LE, "load image file failed!");
		return false;
	}
	const Image* img = image.get()->GetImages();
	// Create texture2d
	D3D11_TEXTURE2D_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	editDescritionStructure(img, desc, subData, SRVDesc);

	if (FAILED(dev->CreateTexture2D(&desc, &subData, m_tex2d.ReleaseAndGetAddressOf()))) {
		MLOG(LL, funcTag, LW, "Create texture2d failed!");
		return false;
	}

	if (FAILED(dev->CreateShaderResourceView(m_tex2d.Get(), &SRVDesc, m_shaderResource.ReleaseAndGetAddressOf()))) {
		MLOG(LL, funcTag, LW, "Create shader resource failed!");
		return false;
	}

	return true;
}

ID3D11ShaderResourceView** CommonTexture::GetSRV() { return m_shaderResource.GetAddressOf(); }

/////////////////////////
// private Function
/////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   HDRTexture   //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
HDRTexture::HDRTexture(const wchar_t* file) : m_source(file) {}

///////////////////
// public function
///////////////////
bool HDRTexture::Setup(ID3D11Device* dev) {
	const static char* funcTag = "HDRTexture::Setup: ";
	// load Image File!
	auto image = std::make_shared<ScratchImage>();
	if (!TextureFactory::GetInstance().LoadHDRTextureFromFile(m_source, image)) {
		MLOG(LL, funcTag, LE, "load image failed!");
		return false;
	}

	const Image* img = image->GetImages();
	//Create texture2d
	D3D11_TEXTURE2D_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;

	editDescritionStructure(img, desc, subData, SRVDesc);
	if (dev->CreateTexture2D(&desc, &subData, m_tex2d.ReleaseAndGetAddressOf())) {
		MLOG(LL, funcTag, LE, "create texture2d failed!");
		return false;
	}
	if (dev->CreateShaderResourceView(m_tex2d.Get(), &SRVDesc, m_shaderResource.ReleaseAndGetAddressOf())) {
		MLOG(LL, funcTag, LE, "create texture2d shader resource view failed!");
		return false;
	}
	return true;
}

ID3D11ShaderResourceView** HDRTexture::GetSRV() { return m_shaderResource.GetAddressOf(); }

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   InternalTexture   ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
InternalTexture::InternalTexture(ID3D11ShaderResourceView* srv) {
	m_shaderResource.Attach(srv);
}

bool InternalTexture::Setup(ID3D11Device* dev) {
	return true;
}

ID3D11ShaderResourceView** InternalTexture::GetSRV() { return m_shaderResource.GetAddressOf(); }

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   QuadDepthTexture   //////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

DepthTexture::DepthTexture(float width, float height, UINT arraySize,
	DXGI_FORMAT bufFormat, DXGI_FORMAT depFormat, DXGI_FORMAT resFormat)
	: width(width), height(height), 
	bufFormat(bufFormat), depFormat(depFormat), resFormat(resFormat),
	m_cubemap(false), m_mipmap(false), m_arraySize(arraySize) {}

bool DepthTexture::Setup(ID3D11Device* dev) {
	const static char* funcTag = "QuadDepthTexture::Setup: ";
	if (width != height && m_cubemap) {
		MLOG(LL, funcTag, LE, "width and height is not equal, can not gen cube map!");
		return false;
	}
	D3D11_TEXTURE2D_DESC texDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC rsvDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	ZeroMemory(&dsDesc, sizeof(dsDesc));
	ZeroMemory(&rsvDesc, sizeof(rsvDesc));

	texDesc.ArraySize = m_arraySize;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = bufFormat;
	texDesc.Height = height;
	texDesc.Width = width;
	texDesc.MipLevels = m_mipmap ? 0 : 1;
	texDesc.MiscFlags = m_cubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;

	if (FAILED(dev->CreateTexture2D(&texDesc, NULL, m_texture.ReleaseAndGetAddressOf()))) {
		MLOG(LL, funcTag, LW, "Create depth texture failed!");
		return false;
	}

	dsDesc.Format = depFormat;
	if (m_arraySize > 1) {
		dsDesc.Texture2DArray.ArraySize = m_arraySize;
		dsDesc.Texture2DArray.FirstArraySlice = 0;
		dsDesc.Texture2DArray.MipSlice = 0;
		dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	}
	else {
		dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsDesc.Texture2D.MipSlice = 0;
	}

	if (FAILED(dev->CreateDepthStencilView(m_texture.Get(), &dsDesc, m_depStenView.ReleaseAndGetAddressOf()))) {
		MLOG(LL, funcTag, LW, "Create depth view failed!");
		return false;
	}

	rsvDesc.Format = resFormat;
	rsvDesc.ViewDimension = m_cubemap ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2D;
	if (m_cubemap) {
		rsvDesc.TextureCube.MipLevels = m_mipmap ? -1 : 1;
		rsvDesc.TextureCube.MostDetailedMip = 0;
	}
	else if (m_arraySize > 1) {
		rsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		rsvDesc.Texture2DArray.ArraySize = m_arraySize;
		rsvDesc.Texture2DArray.FirstArraySlice = 0;
		rsvDesc.Texture2DArray.MipLevels = m_mipmap ? -1 : 1;
		rsvDesc.Texture2DArray.MostDetailedMip = 0;
	}
	else {
		rsvDesc.Texture2D.MipLevels = m_mipmap ? -1 : 1;
		rsvDesc.Texture2D.MostDetailedMip = 0;
	}
	
	if (FAILED(dev->CreateShaderResourceView(m_texture.Get(), &rsvDesc, m_shaderResource.ReleaseAndGetAddressOf()))) {
		MLOG(LL, funcTag, LW, "Create depth shader resource failed!");
		return false;
	}

	return true;
}

void DepthTexture::SetCubeMap() { m_cubemap = true; m_arraySize = 6; }
void DepthTexture::SetMipMap() { m_mipmap = true; }

ID3D11DepthStencilView* DepthTexture::GetDSV() { return m_depStenView.Get(); }
ID3D11ShaderResourceView** DepthTexture::GetSRV() { return m_shaderResource.GetAddressOf(); }

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////   TextureFactory   /////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

TextureFactory::TextureFactory() {
	IWICImagingFactory* pFactory = NULL;
	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)&pFactory
	);
	if (FAILED(hr)) {
		MLOG(LE, "TextureFactory::TextureFactory: Can not initialize WIC Factory!");
		assert(0);
	}
	DirectX::SetWICFactory(pFactory);
}

//////////////////////////
// Static Function
//////////////////////////

TextureFactory& TextureFactory::GetInstance() {
	static TextureFactory _instance;
	return _instance;
}

//////////////////////////
// Public Function
//////////////////////////

bool TextureFactory::LoadCommonTextureFromFile(const wchar_t* file, std::shared_ptr<ScratchImage>& image) {
	if (FAILED(DirectX::LoadFromWICFile(
		file,
		DirectX::WIC_FLAGS_NONE,
		NULL,
		*image,
		NULL ))) {
		MLOG(LW, "TextureFactory::LoadTextureFromFile: failed to load file!");
		return false;
	}
	return true;
}

bool TextureFactory::LoadHDRTextureFromFile(const wchar_t* file, std::shared_ptr<ScratchImage>& image) {
	if (FAILED(DirectX::LoadFromHDRFile(file, NULL, *image))) {
		return false;
	}
	return true;
}
