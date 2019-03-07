#include "DX12Shader.h"
#include "../../Utility/DXCompilerHelper/DXCompilerHelper.h"

namespace UnknownVision {
	void DX12_Shader::updateDescription()
	{
		std::vector<char> err;
		if (!GetDXCompilerHelper()->RetrieveShaderDescriptionFromByteCode(
			m_byteCode, m_description, &err
		)) {
			MLOG(LW, __FUNCTION__, LL, err.data());
		}
	}
}