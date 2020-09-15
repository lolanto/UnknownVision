#include "DXImage.h"
#include "../D3D12/DX12Helpers.h"

BEG_NAME_SPACE

std::unique_ptr<DXImage> DXImage::LoadImageFromFile(const std::string& path) {
	DirectX::ScratchImage image;
	std::wstring wchar_str(path.begin(), path.end());
	assert(SUCCEEDED(DirectX::LoadFromWICFile(wchar_str.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image)));
	std::unique_ptr<DXImage> ret = std::make_unique<DXImage>();
	const DirectX::Image* pImage = image.GetImage(0, 0, 0);
	ret->m_width = pImage->width;
	ret->m_height = pImage->height;
	ret->m_image = std::move(image);
	return ret;
}

std::unique_ptr<DXImage> DXImage::LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ElementFormatType pixelFormat) {
	DirectX::ScratchImage image;
	image.Initialize2D(ElementFormatToDXGIFormat(pixelFormat), width, height, 1, 1);
	size_t bpp = ElementFormatToSizeInByte(pixelFormat);
	auto pImage = image.GetImage(0, 0, 0);
	for (size_t y = 0; y < height; ++y) {
		memcpy(pImage->pixels + pImage->rowPitch * y, imageData + width * bpp * y, width * bpp);
	}
	std::unique_ptr<DXImage> ret = std::make_unique<DXImage>();
	ret->m_width = width;
	ret->m_height = height;
	ret->m_image = std::move(image);
	return ret;
}

void DXImage::Init() {
	assert(SUCCEEDED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED)));
}

void DXImage::Shutdown() {
	CoUninitialize();
}

DXImage::~DXImage()
{
	m_image.Release();
}

bool DXImage::ConvertPixelFormat(ElementFormatType type)
{
	DirectX::ScratchImage newImage;
	assert(SUCCEEDED(DirectX::Convert(m_image.GetImages(), m_image.GetImageCount(), m_image.GetMetadata(),
		ElementFormatToDXGIFormat(type), DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, newImage)));
	m_image.Release();
	m_image = std::move(newImage);
	return true;
}

#if API_TYPE == DX12

std::unique_ptr<Image> Image::LoadImageFromFile(const std::string& path) {
	return DXImage::LoadImageFromFile(path);
}

std::unique_ptr<Image> Image::LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ElementFormatType pixelFormat) {
	return DXImage::LoadImageFromMemory(imageData, width, height, pixelFormat);
}

void Image::Init() {
	DXImage::Init();
}

void Image::Shutdown() {
	DXImage::Shutdown();
}

#endif // API_TYPE == DX12

END_NAME_SPACE
