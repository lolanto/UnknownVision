#include "DX12Shader.h"
#include "../../Utility/InfoLog/InfoLog.h"

BEG_NAME_SPACE

DX12ShaderManager GShaderManager;

ShaderHandle DX12ShaderManager::Compile(const std::filesystem::path& filePath, ShaderType type)
{
	/** TODO: 避免相同Shader多次编译 */
	DXCompilerHelper dxc;
	std::string profile;
	switch (type) {
	case SHADER_TYPE_VERTEX_SHADER: profile = VS_PROFILE; break;
	case SHADER_TYPE_PIXEL_SHADER: profile = PS_PROFILE; break;
	default:
		LOG_WARN("Doesn't support this shader type");
		return ShaderHandle::InvalidIndex();
	}
	DX12Shader newShader;
	bool debugInfo = false;
#ifdef _DEBUG
	debugInfo = true;
#endif // _DEBUG
	if (dxc.CompileToByteCode(filePath.generic_wstring().c_str(), profile.c_str(), newShader.shaderByteCode, debugInfo) == false) {
		LOG_WARN("Compile shader %s failed!", filePath.filename().generic_u8string().c_str());
		LOG_WARN("Error Msg: %s", dxc.LastErrorMsg());
		return ShaderHandle::InvalidIndex();
	}
	/** TODO: 避免相同Shader多次编译 */
	m_shaders.insert(std::make_pair(m_nextShaderHandle, std::move(newShader)));
	return m_nextShaderHandle++;
}

ShaderHandle DX12ShaderManager::Compile(const char* srcCode, ShaderType type, const char* shaderName)
{
	DXCompilerHelper dxc;
	std::string profile;
	switch (type) {
	case SHADER_TYPE_VERTEX_SHADER: profile = VS_PROFILE; break;
	case SHADER_TYPE_PIXEL_SHADER: profile = PS_PROFILE; break;
	default:
		LOG_WARN("Doesn't support this shader type");
		return ShaderHandle::InvalidIndex();
	}
	DX12Shader newShader;
	/** +1是为了涵盖空字符 */
	if (dxc.CompileToByteCode(srcCode, std::strlen(srcCode) + 1, profile.data(), newShader.shaderByteCode, false, shaderName) == false) {
		LOG_WARN("Compile shader failed!");
		LOG_WARN("Error Msg: %s", dxc.LastErrorMsg());
		return ShaderHandle::InvalidIndex();
	}
	/** TODO: 避免相同Shader多次编译 */
	m_shaders.insert({ m_nextShaderHandle, std::move(newShader) });
	return m_nextShaderHandle++;
}

END_NAME_SPACE
