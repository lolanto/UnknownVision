#pragma once
#include "DX12Config.h"
#include "DX12Shader.h"
#include "../RenderSystem/Shader.h"
#include "../RenderSystem/Pipeline.h"

BEG_NAME_SPACE

class DX12GraphicsPipelineObject : public GraphicsPipelineObject {
	friend class DX12PipelineManager;
public:
	DX12GraphicsPipelineObject(VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rasOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList)
		: GraphicsPipelineObject(vs, ps, rasOpt, outputOpt, vtxAttribList) {}
private:
	SmartPTR<ID3D12PipelineState> m_pso;
	SmartPTR<ID3D12RootSignature> m_rs;

};

class DX12PipelineManager {
public:
	/** 构造DX12需要的特定Pipeline object, 包括pso, rs等
	 * @param input 需要build的管线对象
	 * @param vs 和input关联的vertex shader对应的dx数据
	 * @param ps 和input关联的pixel shader对应的dx数据
	 * @return build成功返回true否则返回false */
	bool Build(DX12GraphicsPipelineObject& input, const DX12Shader* vs, const DX12Shader* ps);
private:
	std::list<DX12GraphicsPipelineObject> m_graphicsPSOs;
};

END_NAME_SPACE
