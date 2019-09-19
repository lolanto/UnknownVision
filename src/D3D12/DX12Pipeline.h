#pragma once
#include "DX12Config.h"
#include "DX12Shader.h"
#include "../RenderSystem/Shader.h"
#include "../RenderSystem/Pipeline.h"

BEG_NAME_SPACE

class DX12GraphicsPipelineObject {
public:
	
public:
	SmartPTR<ID3D12PipelineState> m_pso;
	SmartPTR<ID3D12RootSignature> m_rs;
};

class DX12PipelineManager {
public:
	void Build(GraphicsPipeline& input);
};

END_NAME_SPACE
