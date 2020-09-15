#pragma once
#include "DX12Config.h"
#include "DX12DescriptorHeap.h"
#include "../RenderSystem/Shader.h"
#include "../Utility/InfoLog/InfoLog.h"
#include "../Utility/DXCompilerHelper/DXCompilerHelper.h"
#include <map>
#include <string>
BEG_NAME_SPACE

/** 目前仅存储编译字节码数据 */
struct DX12Shader {
	SmartPTR<ID3DBlob> shaderByteCode;
	DX12Shader() = default;
	DX12Shader(DX12Shader&& rhs) {
		shaderByteCode.Swap(rhs.shaderByteCode);
	}
	DX12Shader(const DX12Shader& ref) : shaderByteCode(ref.shaderByteCode) {}
};

/** ShaderManager独立于DX12的其它模块自行运作，其核心工作是负责对Shader文件或者源码提供编译
 * 并且保存编译后的字节码，允许调用者通过Handle索引某一Shader字节码内存
 * 整个APP应该有且仅有一个Manager，由应用保证Manager被删除时，没有任何字节码存储正在被使用 */

class DX12ShaderManager {
public:
	DX12ShaderManager() : m_nextShaderHandle(0) {}
	/** 从源码文件中编译Shader */
	ShaderHandle Compile(const wchar_t* srcFilePath, ShaderType type, const char* shaderName = "");
	/** 从源码字符串编译Shader */
	ShaderHandle Compile(const char* srcCode, ShaderType type, const char* shaderName = "");
	/** 根据Shader句柄获得Shader对象，假如句柄有误返回nullptr */
	const DX12Shader* operator[](ShaderHandle handle) const {
		auto& shaderItem = m_shaders.find(handle);
		if (shaderItem != m_shaders.end())
			return &shaderItem->second;
		else
			return nullptr;
	}
private:
	std::map<ShaderHandle, DX12Shader> m_shaders;
	ShaderHandle m_nextShaderHandle;
};

extern DX12ShaderManager GShaderManager; /**< 负责创建，存储和检索Shader Object */

END_NAME_SPACE
