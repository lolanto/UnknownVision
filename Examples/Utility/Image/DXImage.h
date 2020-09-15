#include "Image.h"
#include <d3d12.h>
#include <DirectXTex/DirectXTex.h>

namespace MImage {

	class DXImage : public Image {
		friend class DX12RenderDevice;
	public:
		static std::unique_ptr<DXImage> LoadImageFromFile(const std::filesystem::path& filePath);
		static std::unique_ptr<DXImage> LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ImageFormat pixelFormat);
		static bool StoreImageToFile(DXImage* input, const std::filesystem::path& outputFilePath);
		static void Init();
		static void Shutdown();
	public:
		virtual ~DXImage();
		virtual bool ConvertPixelFormat(ImageFormat type) override final;
		virtual size_t Width(size_t mipLevel = 0, size_t arrIdx = 0) const;
		virtual size_t Height(size_t mipLevel = 0, size_t arrIdx = 0) const;
		virtual size_t GetRowPitch(size_t mipLevel = 0, size_t arrIdx = 0);
		virtual size_t GetSlicePitch(size_t mipLevel = 0, size_t arrIdx = 0);
		virtual void* GetData(size_t mipLevel = 0, size_t arrIdx = 0);
	private:
		DirectX::ScratchImage m_image;
	};

}
