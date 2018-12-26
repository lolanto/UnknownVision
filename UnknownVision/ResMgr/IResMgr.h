#ifndef IRESOURCE_MANAGER_H
#define IRESOURCE_MANAGER_H

#include "Resource.h"
#include <fstream>
#include <vector>

namespace UnknownVision {
	class ResMgr {
	};

	class TextureMgr : public ResMgr {
	};

	class Texture2DMgr : public TextureMgr {
	public:
		virtual int Create(float width, float height,
			TextureFlag flag, TextureElementType type, uint8_t* data, size_t size) = 0;
		virtual int CreateRenderTarget(float width, float height,
			TextureElementType type) = 0;
		// 创建供vs, ps(fs)使用的纹理资源
		virtual int CreateTexture(float width, float height,
			TextureElementType type, uint8_t* data, size_t size) = 0;
		virtual Texture2D& GetTexture(uint32_t index) = 0;
	};

	class BufferMgr : public ResMgr {
	public:
		virtual int CreateVertexBuffer() = 0;
	};

	class ShaderMgr : public ResMgr {
	public:
		virtual int CreateShaderFromCodeInMemory(ShaderType type, uint8_t* data, size_t size) = 0;
		int CreateShaderFromSoureFile(ShaderType type, const char* fileName) {
			std::ifstream file(fileName, std::ios::in | std::ios::binary);
			if (file.is_open() == false) {
				return -1;
			}
			file.seekg(0, std::ios::end);
			size_t fileSize = file.tellg();
			file.seekg(0, std::ios::beg);
			std::vector<uint8_t> buffer(fileSize);
			file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
			file.close();
			return CreateShaderFromCodeInMemory(type, buffer.data, fileSize);
		}
	};

}

#endif // IRESOURCE_MANAGER_H
