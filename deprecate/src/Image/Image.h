#include "../UVConfig.h"
#include "../UVType.h"
#include <vector>
#include <memory>
#include <string>

BEG_NAME_SPACE
/** 代理class，底层需要根据使用的平台决定具体依赖的对象 */
class Image {
public:
	static std::unique_ptr<Image> LoadImageFromFile(const std::string& path);
	/** 从内存中加载图片数据
	 * Note: 当前默认内存中仅有一张图片(非图片数组，无mipmap)
	 * Note: 图片数据在内存中没有进行任何对齐(一行图片数据 = width * bpp)*/
	static std::unique_ptr<Image> LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ElementFormatType pixelFormat);
	static void Init();
	static void Shutdown();
	friend class DX12RenderDevice;
public:
	virtual ~Image() = default;
	size_t Width() const { return m_width; }
	size_t Height() const { return m_height; }
	virtual bool ConvertPixelFormat(ElementFormatType type) { return false; }
protected:
	size_t m_width;
	size_t m_height;
};

END_NAME_SPACE
