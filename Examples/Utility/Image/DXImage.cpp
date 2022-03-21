#include "DXImage.h"
#include <../Utility/InfoLog/InfoLog.h>

namespace MImage {

	ImageFormat DXGI_FormatToImageFormat(DXGI_FORMAT input) {
		ImageFormat output;
		switch (input) {
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			output = IMAGE_FORMAT_R8G8B8A8;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			output = IMAGE_FORMAT_B8G8R8A8;
			break;
		case DXGI_FORMAT_R8_UNORM:
			output = IMAGE_FORMAT_R8;
			break;
		default:
			LOG_WARN("Doesn't support this format!");
			abort();
		}
		return output;
	}

	DXGI_FORMAT ImageFormatToDXGI_Format(ImageFormat format) {
		switch (format) {
		case IMAGE_FORMAT_R8G8B8A8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case IMAGE_FORMAT_R8:
			return DXGI_FORMAT_R8_UNORM;
		case IMAGE_FORMAT_B8G8R8A8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
		abort();
		return DXGI_FORMAT_UNKNOWN;
	}

	size_t CalculateBPPFromDXGI_FORMAT(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 4;
		}
		abort();
		return 0;
	}

	std::unique_ptr<DXImage> DXImage::LoadImageFromFile(const std::filesystem::path& filePath) {
		DirectX::ScratchImage image;
		auto ext = filePath.extension();
		if (ext.compare(".dds") == 0) {
			if (SUCCEEDED(DirectX::LoadFromDDSFile(filePath.generic_wstring().c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image)) == false) {
				LOG_WARN("Load Image failed, path is: %s", filePath.generic_u8string().c_str());
				return nullptr;
			}
		}
		else if (SUCCEEDED(DirectX::LoadFromWICFile(filePath.generic_wstring().c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image)) == false) {
			LOG_WARN("Load Image failed, path is: %s", filePath.generic_u8string().c_str());
			return nullptr;
		}
		std::unique_ptr<DXImage> ret = std::make_unique<DXImage>();
		const DirectX::Image* pImage = image.GetImage(0, 0, 0);
		ret->m_image = std::move(image);
		ret->m_format = DXGI_FormatToImageFormat(pImage->format);
		return ret;
	}

	std::unique_ptr<DXImage> DXImage::LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ImageFormat pixelFormat) {
		DirectX::ScratchImage image;
		image.Initialize2D(ImageFormatToDXGI_Format(pixelFormat), width, height, 1, 1);
		size_t bpp = CalculateBPPFromDXGI_FORMAT(ImageFormatToDXGI_Format(pixelFormat));
		auto pImage = image.GetImage(0, 0, 0);
		for (size_t y = 0; y < height; ++y) {
			memcpy(pImage->pixels + pImage->rowPitch * y, imageData + width * bpp * y, width * bpp);
		}
		std::unique_ptr<DXImage> ret = std::make_unique<DXImage>();
		ret->m_image = std::move(image);
		ret->m_format = pixelFormat;
		return ret;
	}

	bool MImage::DXImage::StoreImageToFile(DXImage* input, const std::filesystem::path& outputFilePath)
	{
		auto ext = outputFilePath.extension();
		const DirectX::Image& img = *input->m_image.GetImage(0, 0, 0);
		if (ext.compare(".dds") == 0) {
			if (FAILED(DirectX::SaveToDDSFile(img, DirectX::DDS_FLAGS_NONE, outputFilePath.c_str()))) {
				LOG_ERROR("Save file %s failed!", outputFilePath.generic_u8string().c_str());
				return false;
			}
		}
		else {
			GUID guidCode;
			if (ext.compare(".png") == 0) guidCode = DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG);
			else if (ext.compare(".bmp") == 0) guidCode = DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP);
			else if (ext.compare(".jpeg") == 0) guidCode = DirectX::GetWICCodec(DirectX::WIC_CODEC_JPEG);
			else {
				LOG_ERROR("Invalid image file extension %s", ext.generic_u8string().c_str());
				return false;
			}
			if (FAILED(DirectX::SaveToWICFile(img, DirectX::WIC_FLAGS_NONE, guidCode, outputFilePath.c_str()))) {
				LOG_ERROR("Save file %s failed!", outputFilePath.generic_u8string().c_str());
				return false;
			}
		}
		return true;
	}

	void DXImage::Init() {
		if (SUCCEEDED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED)) == false) abort();
	}

	void DXImage::Shutdown() {
		CoUninitialize();
	}

	DXImage::~DXImage()
	{
		m_image.Release();
	}

	bool DXImage::ConvertPixelFormat(ImageFormat type)
	{
		DXGI_FORMAT originalFormat = ImageFormatToDXGI_Format(type);
		const DirectX::Image* pImgs = m_image.GetImages();
		for (int i = 0; i < m_image.GetImageCount(); ++i) {
			if (pImgs[i].format == originalFormat) {
				LOG_WARN("Subimage %d 's format is equal to type. Format convert Failed!", i);
				return true;
			}
		}
		DirectX::ScratchImage newImage;
		if (SUCCEEDED(DirectX::Convert(m_image.GetImages(), m_image.GetImageCount(), m_image.GetMetadata(),
			ImageFormatToDXGI_Format(type), DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, newImage)) == false) {
			LOG_WARN("Image convert pixel format failed");
			return false;
		}
		m_image.Release();
		m_image = std::move(newImage);
		m_format = type;
		return true;
	}

	size_t DXImage::Width(size_t mipLevel, size_t arrIdx) const {
		return m_image.GetImage(mipLevel, arrIdx, 0)->width;
	}

	size_t DXImage::Height(size_t mipLevel, size_t arrIdx) const {
		return m_image.GetImage(mipLevel, arrIdx, 0)->height;
	}

	size_t DXImage::GetRowPitch(size_t mipLevel, size_t arrIdx) {
		return m_image.GetImage(mipLevel, arrIdx, 0)->rowPitch;
	}

	size_t DXImage::GetSlicePitch(size_t mipLevel, size_t arrIdx) {
		return m_image.GetImage(mipLevel, arrIdx, 0)->slicePitch;
	}

	void* DXImage::GetData(size_t mipLevel, size_t arrIdx) {
		return m_image.GetImage(mipLevel, arrIdx, 0)->pixels;
	}

#ifdef DXLike

	std::unique_ptr<Image> Image::LoadImageFromFile(const std::filesystem::path& filePath) {
		return DXImage::LoadImageFromFile(filePath);
	}

	std::unique_ptr<Image> Image::LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ImageFormat pixelFormat) {
		return DXImage::LoadImageFromMemory(imageData, width, height, pixelFormat);
	}

	bool MImage::Image::StoreImageToFile(Image* input, const std::filesystem::path& outputFilePath) {
		return DXImage::StoreImageToFile(dynamic_cast<DXImage*>(input), outputFilePath);
	}

	void Image::Init() {
		DXImage::Init();
	}

	void Image::Shutdown() {
		DXImage::Shutdown();
	}

#endif DXLike

}