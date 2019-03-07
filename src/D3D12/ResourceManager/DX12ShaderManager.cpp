#include "DX12ShaderManager.h"
#include "../../Utility/DXCompilerHelper/DXCompilerHelper.h"

namespace UnknownVision {
	ShaderIdx DX12_ShaderMgr::CreateShaderFromSourceFile(const char * filePath, const char * shaderName,
		ShaderType type)
	{
		const auto& nameIter = m_shaderNameToShaderIdx.find(shaderName);
		if (nameIter != m_shaderNameToShaderIdx.end()) {
			MLOG(LE, __FUNCTION__, LL, "shader name has been occupied!");
			return ShaderIdx(-1);
		}
		std::string shaderProfile;
		switch (type)
		{
		case UnknownVision::SHADER_TYPE_VERTEX_SHADER:
			shaderProfile = "vs_6_0";
			break;
		case UnknownVision::SHADER_TYPE_PIXEL_SHADER:
			shaderProfile = "ps_6_0";
			break;
		case UnknownVision::SHADER_TYPE_GEOMETRY_SHADER:
			shaderProfile = "gs_6_0";
			break;
		case UnknownVision::SHADER_TYPE_COMPUTE_SHADER:
			shaderProfile = "cs_6_0";
			break;
		default:
			MLOG(LE, __FUNCTION__, LL, "invalid shader type!");
			return ShaderIdx(-1);
			break;
		}
		SmartPTR<ID3DBlob> byteCode;
		std::vector<char> err;
		bool outputDebugInfo = false;
#ifdef _DEBUG
		outputDebugInfo = true;
#endif // _DEBUG

		if (!GetDXCompilerHelper()->CompileToByteCode(filePath, 
			shaderProfile.c_str(), byteCode, outputDebugInfo, &err)) {
			MLOG(LW, __FUNCTION__, LL, "compile shader failed!");
			return ShaderIdx(-1);
		}
		DX12_Shader shader(shaderName, type, RID_COUNT++);
		shader.m_byteCode = byteCode;
		m_shaderIdxToShader.insert(std::make_pair(shader.RID, shader));
		m_filePathToShaderIdx.insert(std::make_pair(filePath, shader.RID));
		m_shaderNameToShaderIdx.insert(std::make_pair(shaderName, shader.RID));
		return ShaderIdx(shader.RID);
	}

	Shader & DX12_ShaderMgr::GetShaderFromIndex(ShaderIdx index)
	{
		auto& shaderIter = m_shaderIdxToShader.find(index);
		if (shaderIter == m_shaderIdxToShader.end()) {
			throw(std::out_of_range("invalid shader index"));
		}
		return shaderIter->second;
	}

	const Shader & DX12_ShaderMgr::GetShaderFromIndex(ShaderIdx index) const
	{
		const auto& shaderIter = m_shaderIdxToShader.find(index);
		if (shaderIter == m_shaderIdxToShader.end()) {
			throw(std::out_of_range("invalid shader index"));
		}
		return shaderIter->second;
	}

	Shader & DX12_ShaderMgr::GetShaderFromName(const char * name)
	{
		auto& nameIter = m_shaderNameToShaderIdx.find(name);
		if (nameIter == m_shaderNameToShaderIdx.end()) {
			throw(std::out_of_range("invalid shader name"));
		}
		auto& shaderIter = m_shaderIdxToShader.find(nameIter->second);
		return shaderIter->second;
	}

	const Shader & DX12_ShaderMgr::GetShaderFromName(const char * name) const
	{
		auto& nameIter = m_shaderNameToShaderIdx.find(name);
		if (nameIter == m_shaderNameToShaderIdx.end()) {
			throw(std::out_of_range("invalid shader name"));
		}
		const auto& shaderIter = m_shaderIdxToShader.find(nameIter->second);
		return shaderIter->second;
	}
}
