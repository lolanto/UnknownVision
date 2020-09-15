#include "Image.h"
#include <d3d12.h>
#include <DirectXTex.h>

BEG_NAME_SPACE

class DXImage : public Image {
	friend class DX12RenderDevice;
public:
	static std::unique_ptr<DXImage> LoadImageFromFile(const std::string& path);
	static std::unique_ptr<DXImage> LoadImageFromMemory(const uint8_t* imageData, size_t width, size_t height, ElementFormatType pixelFormat);
	static void Init();
	static void Shutdown();
public:
	virtual ~DXImage();
	virtual bool ConvertPixelFormat(ElementFormatType type) override final;
private:
	DirectX::ScratchImage m_image;
};

END_NAME_SPACE
