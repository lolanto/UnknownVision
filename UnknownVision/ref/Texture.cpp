#include <wincodec.h>
#include <memory>
#include "Texture.h"
#include "InfoLog.h"

using DirectX::Image;
using DirectX::ScratchImage;

bool editDescritionStructure(const ScratchImage* mainData,
	ID3D11Device* dev,
	Microsoft::WRL::ComPtr<ID3D11Texture2D>& tex2d,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv) {
	
	D3D11_TEXTURE2D_DESC tex2dDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC tex2dSRVDesc;
	ZeroMemory(&tex2dDesc, sizeof(tex2dDesc));

	auto img = mainData->GetImages();
	auto metaData = mainData->GetMetadata();
	// set texture2d desc
	tex2dDesc.ArraySize = 1;
	tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.Format = img->format;
	tex2dDesc.Height = img->height;
	tex2dDesc.Width = img->width;
	// 使用全套mipmap
	tex2dDesc.MipLevels = metaData.mipLevels;
	// 不使用多重采样抗锯齿
	tex2dDesc.SampleDesc.Count = 1;
	tex2dDesc.SampleDesc.Quality = 0;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;

	std::vector<D3D11_SUBRESOURCE_DATA> subData;
	// set texture2d subresource data
	for (short i = 0; i < metaData.mipLevels; ++i) {
		auto mipImage = mainData->GetImage(i, 0, 0);
		D3D11_SUBRESOURCE_DATA ele;
		ele.pSysMem = mipImage->pixels;
		ele.SysMemPitch = mipImage->rowPitch;
		ele.SysMemSlicePitch = mipImage->slicePitch;
		subData.push_back(ele);
	}

	// create shader resource view
	tex2dSRVDesc.Format = img->format;
	tex2dSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	tex2dSRVDesc.Texture2D.MipLevels = -1;
	tex2dSRVDesc.Texture2D.MostDetailedMip = 0;

	if (FAILED(dev->CreateTexture2D(&tex2dDesc, &subData[0], tex2d.ReleaseAndGetAddressOf()))) {
		return false;
	}
	if (FAILED(dev->CreateShaderResourceView(tex2d.Get(), &tex2dSRVDesc, srv.ReleaseAndGetAddressOf()))) {
		return false;
	}
	return true;
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

	// Create texture2d
	if (!editDescritionStructure(image.get(), dev, m_tex2d, m_srv_tex)) {
		MLOG(LW, __FUNCTION__, LL, " create texture2d failed!");
		return false;
	}
	return true;
}

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

	//Create texture2d
	// Create texture2d
	if (!editDescritionStructure(image.get(), dev, m_tex2d, m_srv_tex)) {
		MLOG(LW, __FUNCTION__, LL, " create texture2d failed!");
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   DDSTexture   //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

DDSTexture::DDSTexture(const wchar_t* file) : m_source(file) {}

bool DDSTexture::Setup(ID3D11Device* dev) {
	// load image file!
	auto image = std::make_shared<ScratchImage>();
	if (!TextureFactory::GetInstance().LoadDDSTextureFromFile(m_source, image)) {
		MLOG(LW, __FUNCTION__, LL, " Load dds file failed!");
		return false;
	}

	// Create Texture 2d
	// Create texture2d
	if (!editDescritionStructure(image.get(), dev, m_tex2d, m_srv_tex)) {
		MLOG(LW, __FUNCTION__, LL, " create texture2d failed!");
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   DDSTextureArray  /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

//DDSTextureArray::DDSTextureArray(const wchar_t* path) {
//	m_sources.push_back(path);
//}

bool DDSTextureArray::Setup(ID3D11Device* dev) {
	std::vector<std::shared_ptr<ScratchImage>> images;
	for (auto iter : m_sources) {
		auto eleImage = std::make_shared<ScratchImage>();
		if (!TextureFactory::GetInstance().LoadDDSTextureFromFile(iter, eleImage)) {
			MLOG(LW, __FUNCTION__, LL, " Load dds file failed!");
			return false;
		}
		images.push_back(eleImage);
	}
	D3D11_TEXTURE2D_DESC tex2dDesc;
	ZeroMemory(&tex2dDesc, sizeof(tex2dDesc));
	auto tmpMetaData = images[0]->GetMetadata();
	tex2dDesc.ArraySize = m_sources.size();
	tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.Format = tmpMetaData.format;
	tex2dDesc.Height = tmpMetaData.height;
	tex2dDesc.Width = tmpMetaData.width;
	// 不使用Mipmap
	tex2dDesc.MipLevels = 1;
	tex2dDesc.MiscFlags = 0;
	// 不使用multisample
	tex2dDesc.SampleDesc.Count = 1;
	tex2dDesc.SampleDesc.Quality = 0;

	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
	
	std::vector<D3D11_SUBRESOURCE_DATA> subDatas;
	for (auto iter : images) {
		auto img = iter->GetImages();
		D3D11_SUBRESOURCE_DATA ele;
		ele.pSysMem = img->pixels;
		ele.SysMemPitch = img->rowPitch;
		ele.SysMemSlicePitch = img->slicePitch;
		subDatas.push_back(ele);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = tmpMetaData.format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.ArraySize = m_sources.size();
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.MostDetailedMip = 0;

	if (FAILED(dev->CreateTexture2D(&tex2dDesc, &subDatas[0], m_tex2d.ReleaseAndGetAddressOf()))) {
		MLOG(LE, __FUNCTION__, LL, " create texture2d failed!");
		return false;
	}
	if (FAILED(dev->CreateShaderResourceView(m_tex2d.Get(), &srvDesc, m_srv_tex.ReleaseAndGetAddressOf()))) {
		MLOG(LE, __FUNCTION__, LL, "create shader resource view failed!");
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   QuadDepthTexture   //////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

DepthTexture::DepthTexture(float width, float height, bool hasMipmap, UINT arraySize,
	DXGI_FORMAT bufFormat, DXGI_FORMAT depFormat, DXGI_FORMAT resFormat)
	: width(width), height(height), 
	bufFormat(bufFormat), depFormat(depFormat), resFormat(resFormat),
	hasMipMap(hasMipMap), m_arraySize(arraySize) {}

void DepthTexture::preSetDesc() {
	ZeroMemory(&m_texDesc, sizeof(m_texDesc));
	ZeroMemory(&m_dsDesc, sizeof(m_dsDesc));
	ZeroMemory(&m_rsvDesc, sizeof(m_rsvDesc));

	m_texDesc.ArraySize = m_arraySize;
	m_texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	m_texDesc.CPUAccessFlags = 0;
	m_texDesc.Format = bufFormat;
	m_texDesc.Height = height;
	m_texDesc.Width = width;
	m_texDesc.MipLevels = hasMipMap ? 0 : 1;
	m_texDesc.SampleDesc.Count = 1;
	m_texDesc.SampleDesc.Quality = 0;
	m_texDesc.Usage = D3D11_USAGE_DEFAULT;

	m_dsDesc.Format = depFormat;
	if (m_arraySize > 1) {
		m_dsDesc.Texture2DArray.ArraySize = m_arraySize;
		m_dsDesc.Texture2DArray.FirstArraySlice = 0;
		m_dsDesc.Texture2DArray.MipSlice = 0;
		m_dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	}
	else {
		m_dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		m_dsDesc.Texture2D.MipSlice = 0;
	}

	m_rsvDesc.Format = resFormat;
	m_rsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	if (m_arraySize > 1) {
		m_rsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		m_rsvDesc.Texture2DArray.ArraySize = m_arraySize;
		m_rsvDesc.Texture2DArray.FirstArraySlice = 0;
		m_rsvDesc.Texture2DArray.MipLevels = hasMipMap ? -1 : 1;
		m_rsvDesc.Texture2DArray.MostDetailedMip = 0;
	}
	else {
		m_rsvDesc.Texture2D.MipLevels = hasMipMap ? -1 : 1;
		m_rsvDesc.Texture2D.MostDetailedMip = 0;
	}
}

bool DepthTexture::Setup(ID3D11Device* dev) {
	preSetDesc();
	if (FAILED(dev->CreateTexture2D(&m_texDesc, NULL, m_texture.ReleaseAndGetAddressOf()))) {
		MLOG(LL, __FUNCTION__, LW, "Create depth texture failed!");
		return false;
	}

	if (FAILED(dev->CreateDepthStencilView(m_texture.Get(), &m_dsDesc, m_dsv.ReleaseAndGetAddressOf()))) {
		MLOG(LL, __FUNCTION__, LW, "Create depth view failed!");
		return false;
	}

	if (FAILED(dev->CreateShaderResourceView(m_texture.Get(), &m_rsvDesc, m_srv_tex.ReleaseAndGetAddressOf()))) {
		MLOG(LL, __FUNCTION__, LW, "Create depth shader resource failed!");
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   CubeMapDepthTexture   //////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CubeMapDepthTexture::CubeMapDepthTexture(float size, bool hasMipMap, 
	DXGI_FORMAT bufFormat, DXGI_FORMAT depFormat, DXGI_FORMAT resFormat)
	: DepthTexture(size, size, hasMipMap, 6, bufFormat, depFormat,resFormat) {}

bool CubeMapDepthTexture::Setup(ID3D11Device * dev)
{
	preSetDesc();
	return DepthTexture::Setup(dev);
}

void CubeMapDepthTexture::preSetDesc()
{
	DepthTexture::preSetDesc();
	m_texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	m_rsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	m_rsvDesc.TextureCube.MipLevels = hasMipMap ? -1 : 1;
	m_rsvDesc.TextureCube.MostDetailedMip = 0;
}


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

bool TextureFactory::LoadDDSTextureFromFile(const wchar_t * file, std::shared_ptr<DirectX::ScratchImage>& image)
{
	if (FAILED(DirectX::LoadFromDDSFile(file, DirectX::DDS_FLAGS_NONE, nullptr, *image))) {
		return false;
	}
	return true;
}
