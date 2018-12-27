#ifndef D3D11_SHADER_H
#define D3D11_SHADER_H

#include "../DX11_UVConfig.h"
namespace UnknownVision {
	class DX11_Shader : public Shader {
	public:
		DX11_Shader(ShaderType type, 
			SmartPTR<ID3D11DeviceChild> shader, uint32_t RID)
			: m_shader(shader), Shader(type, RID) {}
		// Vertex Shader需要使用编译后的代码生成input layout
		ID3DBlob* ByteCode() const { return m_blob.Get(); }
		void SetByteCode(SmartPTR<ID3DBlob>& blob) { m_blob.Swap(blob); }

		ID3D11VertexShader* VertexShader() const { 
			ID3D11VertexShader* ret = nullptr;
			if (FAILED(m_shader->QueryInterface<ID3D11VertexShader>(&ret))) {
				return nullptr;
			}
			return ret;
		}
		ID3D11PixelShader* PixelShader() const {
			ID3D11PixelShader* ret = nullptr;
			if (FAILED(m_shader->QueryInterface<ID3D11PixelShader>(&ret))) {
				return nullptr;
			}
			return ret;
		}
	private:
		SmartPTR<ID3D11DeviceChild> m_shader;
		SmartPTR<ID3DBlob> m_blob;
	};
}

#endif // D3D11_SHADER_H
