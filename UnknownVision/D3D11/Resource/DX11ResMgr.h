#ifndef D3D11_RESOURCE_MANAGER_H
#define D3D11_RESOURCE_MANAGER_H

#include "../../ResMgr/IResMgr.h"
#include "DX11Texture.h"
#include "DX11Shader.h"
#include <vector>
#include <cassert>
namespace UnknownVision {
	class DX11_Texture2DMgr : public Texture2DMgr {
	public:
		DX11_Texture2DMgr(ID3D11Device* dev) : m_dev(dev) {};
	public:
		int Create(float width, float height,
			TextureFlag flag, TextureElementType type, uint8_t* data, size_t size);
		int CreateRenderTarget(float width, float height,
			TextureElementType type);
		int CreateTexture(float width, float height,
			TextureElementType type, uint8_t* data, size_t bytePerLine);
		int CreateDepthStencilTexture(float width, float height,
			TextureElementType type);
		Texture2D& GetTexture(uint32_t index) {
			assert(index < m_texs.size() && index >= 0);
			return m_texs[index];
		}
	private:
		// 创建基础纹理描述，需要设置bindFlag!
		D3D11_TEXTURE2D_DESC createTexDesc(float width, float height,
			TextureElementType type);
	private:
		std::vector<DX11_Texture2D> m_texs;
		ID3D11Device*						m_dev = nullptr;
	};

	class DX11_ShaderMgr : public ShaderMgr {
	public:
		DX11_ShaderMgr(ID3D11Device* dev) : m_dev(dev) {}
	public:
		int CreateShaderFromCodeInMemory(ShaderType type, uint8_t* data, size_t size);
	private:
		std::vector<DX11_Shader> m_shaders;
		ID3D11Device * m_dev = nullptr;
	};
}

#endif // D3D11_RESOURCE_MANAGER_H
