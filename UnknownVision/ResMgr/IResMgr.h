#ifndef IRESOURCE_MANAGER_H
#define IRESOURCE_MANAGER_H

#include "Resource.h"

namespace UnknownVision {
	class ResMgr {
	};

	class TextureMgr : public ResMgr {
	public:
		
	};

	class Texture2DMgr : public TextureMgr {
	public:
		virtual Texture2D & Create(float width, float height,
			TextureFlag flag, TextureElementType type, uint8_t* data, size_t size) = 0;
		virtual Texture2D & CreateRenderTarget(float width, float height,
			TextureElementType type) = 0;
		virtual Texture2D & CreateTexture(float width, float height,
			TextureElementType type, uint8_t* data, size_t size) = 0;
		virtual Texture2D & GetTexture(uint32_t index) = 0;
	};
}

#endif // IRESOURCE_MANAGER_H
