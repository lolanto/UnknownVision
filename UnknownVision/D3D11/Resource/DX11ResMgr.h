#ifndef D3D11_RESOURCE_MANAGER_H
#define D3D11_RESOURCE_MANAGER_H

#include "../../ResMgr/IResMgr.h"
#include "DX11Texture.h"
#include "DX11Shader.h"
#include "DX11Buffer.h"
#include <vector>
#include <cassert>
namespace UnknownVision {
	class DX11_Texture2DMgr : public Texture2DMgr {
	public:
		DX11_Texture2DMgr(ID3D11Device* dev) : m_dev(dev) {};
		virtual ~DX11_Texture2DMgr() {}
	public:
		int Create(float width, float height,
			uint32_t flag, TextureElementType type, uint8_t* data, size_t size);
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
		virtual ~DX11_ShaderMgr() {}
	public:
		int CreateShaderFromBinaryFile(ShaderType type, const char* fileName);
		Shader& GetShader(uint32_t index) {
			assert(index < m_shaders.size() && index >= 0);
			return m_shaders[index];
		}
	private:
		std::vector<DX11_Shader> m_shaders;
		ID3D11Device * m_dev = nullptr;
	};

	class DX11_BufferMgr : public BufferMgr {
	public:
		DX11_BufferMgr(ID3D11Device* dev) : m_dev(dev) {}
		virtual ~DX11_BufferMgr() {}
	public:
		int CreateBuffer(size_t size, size_t numEle, uint32_t flag, uint8_t* data);
		int CreateVertexBuffer(size_t numVtxs, size_t vtxSize, uint8_t* data);
		int CreateConstantBuffer(size_t size, uint8_t* data, uint32_t flag = 0);
		Buffer & GetBuffer(uint32_t index) {
			assert(index < m_buffers.size() && index >= 0);
			return m_buffers[index];
		}
	private:
		std::vector<DX11_Buffer> m_buffers;
		ID3D11Device* m_dev = nullptr;
	};

} // namespace UnknownVision

#endif // D3D11_RESOURCE_MANAGER_H
