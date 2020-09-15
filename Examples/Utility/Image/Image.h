#include <vector>
#include <memory>
#include <string>
#include <filesystem>

#define DXLike
namespace MImage {
	enum ImageFormat : uint8_t {
		IMAGE_FORMAT_R8G8B8A8,
		IMAGE_FORMAT_B8G8R8A8,
		IMAGE_FORMAT_R8
	};

	/** 代理class，底层需要根据使用的平台决定具体依赖的对象 */
	class Image {
	public:
		static std::unique_ptr<Image> LoadImageFromFile(const std::filesystem::path& filePath);
		/** 从内存中加载图片数据
		 * Note: 当前默认内存中仅有一张图片(非图片数组，无mipmap)
		 * Note: 图片数据在内存中没有进行任何对齐(一行图片数据 = width * bpp)*/
		static std::unique_ptr<Image> LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ImageFormat pixelFormat);
		static bool StoreImageToFile(Image* input, const std::filesystem::path& outputFilePath);
		static void Init();
		static void Shutdown();
	public:
		virtual ~Image() = default;
		virtual size_t Width(size_t mipLevel = 0, size_t arrIdx = 0) const { return 0; }
		virtual size_t Height(size_t mipLevel = 0, size_t arrIdx = 0) const { return 0; }
		virtual bool ConvertPixelFormat(ImageFormat type) { return false; }
		/** TODO: 暂时不支持image array，subresourceIdx只图片中的mipmap层级 */
		virtual size_t GetRowPitch(size_t mipLevel = 0, size_t arrIdx = 0) { return 0; }
		virtual size_t GetSlicePitch(size_t mipLevel = 0, size_t arrIdx = 0) { return 0; }
		virtual void* GetData(size_t mipLevel = 0, size_t arrIdx = 0) { return nullptr; }
		ImageFormat GetFormat() const { return m_format; }
	protected:
		ImageFormat m_format;
	};

}
