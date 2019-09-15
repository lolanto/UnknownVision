#pragma once
#include "DX12Config.h"
#include "DX12DescriptorHeap.h"
#include "../RenderSystem/Shader.h"
#include "../Utility/InfoLog/InfoLog.h"
#include "../Utility/DXCompilerHelper/DXCompilerHelper.h"
#include <map>
#include <string>
BEG_NAME_SPACE

struct DX12Shader {
	SmartPTR<ID3DBlob> shaderByteCode;
	std::map<std::string, D3D12_DESCRIPTOR_RANGE> bindingInfo;

};

class DX12ShaderManager {
public:
	bool Compile(const char* src);
	const DX12Shader* operator[](ShaderHandle handle) const {
		auto& shaderItem = m_shaders.find(handle);
		if (shaderItem != m_shaders.end())
			return &(shaderItem->second);
		else
			return nullptr;
	}
private:
	std::map<ShaderHandle, DX12Shader> m_shaders;
};

/** 负责将参数传送到GPU  */
class DX12ParameterPipeline : public ParameterPipeline {
public:
	virtual bool SendPackage(CommandUnit& cu, std::vector<ParameterPackage> packages);
private:
	TransientDX12DescriptorHeap m_descHeap;
};

END_NAME_SPACE
