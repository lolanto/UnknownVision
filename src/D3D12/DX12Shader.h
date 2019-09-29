#pragma once
#include "DX12Config.h"
#include "DX12DescriptorHeap.h"
#include "../RenderSystem/Shader.h"
#include "../Utility/InfoLog/InfoLog.h"
#include "../Utility/DXCompilerHelper/DXCompilerHelper.h"
#include <map>
#include <string>
BEG_NAME_SPACE

extern D3D12_SAMPLER_DESC DefaultSamplerDesc; /**< 用来应对没有给某个采样器提供具体设置时提供的默认设置 */

struct DescriptorHeapRequirementDesc {
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	size_t size;
};

struct DX12Shader {
	SmartPTR<ID3DBlob> shaderByteCode;
	std::map<std::string, D3D12_DESCRIPTOR_RANGE> ranges; /**< 参数的名称和其在当前shader中的绑定关系 */

	DX12Shader() = default;
	DX12Shader(DX12Shader&& rhs) {
		shaderByteCode.Swap(rhs.shaderByteCode);
		ranges.swap(rhs.ranges);
	}
	DX12Shader(const DX12Shader& ref) : shaderByteCode(ref.shaderByteCode), ranges(ref.ranges) {}
};

class DX12ShaderManager {
public:
	DX12ShaderManager() : m_nextShaderHandle(0) {}
	ShaderHandle Compile(const wchar_t* srcFilePath, ShaderType type);
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
