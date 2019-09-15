#pragma once
#include "DX12Config.h"
#include "DX12DescriptorHeap.h"
#include "../RenderSystem/Pipeline.h"

#include <map>
#include <array>
BEG_NAME_SPACE

struct DX12GraphicsPipelineObject {
	SmartPTR<ID3D12PipelineState> pso;
	size_t numDescriptors;
	BasicDX12DescriptorHeap::HeapBlock block;
	std::array<uint8_t, SHADER_TYPE_NUMBER_OF_TYPE> shaderParameterHeapOffset;
	std::map<std::string, size_t> parameterOffsetOfHeap;
};

class DX12PipelineManager {
public:
private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC createGraphPSODesc(const GraphicsPipeline& pipeline);
	CD3DX12_ROOT_SIGNATURE_DESC createRootSignatureDesc(const GraphicsPipeline& pipeline, DX12GraphicsPipelineObject& output);
private:
	std::map<uint64_t, DX12GraphicsPipelineObject> m_psoStorage;
};

END_NAME_SPACE
