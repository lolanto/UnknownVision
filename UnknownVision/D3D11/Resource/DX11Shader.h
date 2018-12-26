#ifndef D3D11_SHADER_H
#define D3D11_SHADER_H

#include "../DX11_UVConfig.h"
namespace UnknownVision {
	class DX11_Shader : public Shader {
	public:
		DX11_Shader(ShaderType type, 
			SmartPTR<ID3D11DeviceChild> shader, uint32_t RID)
			: m_shader(shader), Shader(type, RID) {}
	private:
		SmartPTR<ID3D11DeviceChild> m_shader;
	};
}

#endif // D3D11_SHADER_H
