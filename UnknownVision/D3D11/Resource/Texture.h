#ifndef D3D11_TEXTURE_H
#define D3D11_TEXTURE_H

#include "../../ResMgr/Resource.h"
namespace UnknownVision {
	class D3D11_Texture2D : public UnknownVision::Texture2D {
	public:
		D3D11_Texture2D(float width, float height,
			TextureFlag flag, TextureElementType type, UINT RID)
			: UnknownVision::Texture2D(width, height, flag, type, RID) {}
	};
}

#endif // D3D11_TEXTURE_H
