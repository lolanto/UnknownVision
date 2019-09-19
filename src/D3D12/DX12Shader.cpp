#include "DX12Shader.h"

BEG_NAME_SPACE

ShaderHandle DX12ShaderManager::Compile(const wchar_t* src, ShaderType type)
{
	/** TODO: 避免相同Shader多次编译 */
	DXCompilerHelper dxc;
	std::string profile;
	switch (type) {
	case SHADER_TYPE_VERTEX_SHADER: profile = "vs_6_0"; break;
	case SHADER_TYPE_PIXEL_SHADER: profile = "ps_6_0"; break;
	default:
		assert(false);
	}
	DX12Shader newShader;
	assert(dxc.CompileToByteCode(src, profile.c_str(), newShader.shaderByteCode));
	auto&& reflection = dxc.RetrieveShaderDescriptionFromByteCode(newShader.shaderByteCode);
	D3D12_SHADER_DESC desc;
	assert(SUCCEEDED(reflection->GetDesc(&desc)));
	/** 分析出该Shader中会被使用的参数，名称以及绑定方式 */
	/** 需要处理Sampler的情形，凡是遇到Sampler，通通使用静态配置 */
	/** Shader里面的参数对应关系最后会由每个具体的BasicShader中的参数列表进行组合
	 * 每个ParameterGroup都有一个顺序的参数列表，每个参数参数列表都会对应一个root signature的设置以及若个Descriptor heap设置*/
}

END_NAME_SPACE
