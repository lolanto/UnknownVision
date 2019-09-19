#include "DX12PipelineManager.h"
#include "DX12RenderDevice.h"
#include "DX12Shader.h"
#include "DX12Helpers.h"

BEG_NAME_SPACE

extern DX12ShaderManager GShaderManager;


D3D12_GRAPHICS_PIPELINE_STATE_DESC
DX12PipelineManager::createGraphPSODesc(const GraphicsPipeline& pipeline) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	desc.BlendState = AnalyseBlendingOptionsFromOutputStageOptions(pipeline.outputOpt);
	
}

CD3DX12_ROOT_SIGNATURE_DESC
createRootSignatureDesc(const GraphicsPipeline& pipeline, DX12GraphicsPipelineObject& output) {
	CD3DX12_ROOT_SIGNATURE_DESC desc;
	size_t heapIndex = 0;
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE> > ranges;
	std::vector<CD3DX12_ROOT_PARAMETER > parameters;
	auto initDesc = [&ranges, &parameters, &heapIndex, &output](BasicShader* shader, D3D12_SHADER_VISIBILITY visibility)
	{
		ranges.emplace_back();
		auto shaderObject = GShaderManager[shader->GetHandle()];
		assert(shaderObject != nullptr);
		for (auto& binding : shaderObject->bindingInfo) {
			output.parameterOffsetOfHeap.insert(std::make_pair(binding.first, heapIndex++));
			ranges.back().push_back(binding.second);
		}
		parameters.emplace_back();
		parameters.back().InitAsDescriptorTable(ranges.back().size(), ranges.back().data(), visibility);
	};
	initDesc(pipeline.vs, D3D12_SHADER_VISIBILITY_VERTEX);
	initDesc(pipeline.ps, D3D12_SHADER_VISIBILITY_PIXEL);
	desc.Init(parameters.size(), parameters.data());
	return desc;
	
}

END_NAME_SPACE
