#ifndef D3D11_TEXTURE_H
#define D3D11_TEXTURE_H

#include "../DX11_UVConfig.h"

namespace UnknownVision {
	class DX11_Texture2D : public UnknownVision::Texture2D {
	public:
		DX11_Texture2D(float width, float height,
			SmartPTR<ID3D11Texture2D> ptr,
			TextureFlagCombination flags, TextureElementType type, UINT RID)
			: m_tex(ptr), UnknownVision::Texture2D(width, height, flags, type, RID) {}

		ID3D11Texture2D* Texture() const { return m_tex.Get(); }
	private:
		SmartPTR<ID3D11Texture2D> m_tex; // 实际存储贴图的对象
	};
}

#endif // D3D11_TEXTURE_H
