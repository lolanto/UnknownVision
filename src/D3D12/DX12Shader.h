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
	std::map<std::string, D3D12_DESCRIPTOR_RANGE> range; /**< 参数的名称和其在当前shader中的绑定关系 */
};

class DX12ShaderManager {
public:
	ShaderHandle Compile(const wchar_t* src, ShaderType type);
	const DX12Shader* operator[](ShaderHandle handle) const {
		auto& shaderItem = m_shaders.find(handle);
		if (shaderItem != m_shaders.end())
			return &shaderItem->second;
		else
			return nullptr;
	}
private:
	std::map<ShaderHandle, DX12Shader> m_shaders;
	
};


END_NAME_SPACE
