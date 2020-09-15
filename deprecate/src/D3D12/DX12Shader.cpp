#include "DX12Shader.h"

BEG_NAME_SPACE

DX12ShaderManager GShaderManager;

ShaderHandle DX12ShaderManager::Compile(const wchar_t* srcFilePath, ShaderType type, const char* shaderName)
{
	/** TODO: 避免相同Shader多次编译 */
	DXCompilerHelper dxc;
	std::string profile;
	switch (type) {
	case SHADER_TYPE_VERTEX_SHADER: profile = "vs_5_1"; break;
	case SHADER_TYPE_PIXEL_SHADER: profile = "ps_5_1"; break;
	default:
		assert(false);
	}
	DX12Shader newShader;
	assert(dxc.CompileToByteCode(srcFilePath, profile.c_str(), newShader.shaderByteCode));
	/** TODO: 避免相同Shader多次编译 */
	m_shaders.insert(std::make_pair(m_nextShaderHandle, std::move(newShader)));
	return m_nextShaderHandle++;
}

ShaderHandle DX12ShaderManager::Compile(const char* srcCode, ShaderType type, const char* shaderName)
{
	DXCompilerHelper dxc;
	std::string profile;
	switch (type) {
	case SHADER_TYPE_VERTEX_SHADER: profile = "vs_5_1"; break;
	case SHADER_TYPE_PIXEL_SHADER: profile = "ps_5_1"; break;
	default:
		assert(false);
	}
	DX12Shader newShader;
	/** +1是为了涵盖空字符 */
	assert(dxc.CompileToByteCode(srcCode, std::strlen(srcCode) + 1, profile.data(), newShader.shaderByteCode, false, shaderName));
	/** TODO: 避免相同Shader多次编译 */
	m_shaders.insert({ m_nextShaderHandle, std::move(newShader) });
	return m_nextShaderHandle++;
}

END_NAME_SPACE
