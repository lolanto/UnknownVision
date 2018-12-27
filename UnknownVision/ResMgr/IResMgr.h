#ifndef IRESOURCE_MANAGER_H
#define IRESOURCE_MANAGER_H

#include "Resource.h"
#include <fstream>
#include <vector>

namespace UnknownVision {
	class ResMgr {
	public:
		virtual ~ResMgr() {}
	};

	class TextureMgr : public ResMgr {
	public:
		virtual ~TextureMgr() {}
	};

	class Texture2DMgr : public TextureMgr {
	public:
		virtual ~Texture2DMgr() {}
	public:
		virtual int Create(float width, float height,
			uint32_t flag, TextureElementType type, uint8_t* data, size_t size) = 0;
		virtual int CreateRenderTarget(float width, float height,
			TextureElementType type) = 0;
		// 创建供vs, ps(fs)使用的纹理资源
		virtual int CreateTexture(float width, float height,
			TextureElementType type, uint8_t* data, size_t size) = 0;
		virtual Texture2D& GetTexture(uint32_t index) = 0;
	};

	class BufferMgr : public ResMgr {
	public:
		virtual ~BufferMgr() {}
	public:
		// create Raw buffer
		virtual int CreateBuffer(size_t size, size_t numEle, uint32_t flag, uint8_t* data = nullptr) = 0;
		virtual int CreateVertexBuffer(size_t numVtx, size_t vtxSize, uint8_t* data) = 0;
		virtual Buffer& GetBuffer(uint32_t index) = 0;
	};

	class ShaderMgr : public ResMgr {
	public:
		virtual ~ShaderMgr() {}
	public:
		// 直接加载已经编译过的shader
		virtual int CreateShaderFromBinaryFile(ShaderType type, const char* fileName) = 0;
		virtual Shader& GetShader(uint32_t index) = 0;
	};

}

#endif // IRESOURCE_MANAGER_H
