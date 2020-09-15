#pragma once
#include "DX12Config.h"
#include "../GraphicsInterface/Pipeline.h"

#include <map>
#include <unordered_map>
#include <string>

BEG_NAME_SPACE

class DX12Shader;

class DX12GraphicsPipelineObject : public GraphicsPipelineObject {
	friend class DX12PipelineManager;
public:
	DX12GraphicsPipelineObject(VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rasOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList)
		: GraphicsPipelineObject(vs, ps, rasOpt, outputOpt, vtxAttribList) {}
	DX12GraphicsPipelineObject(DX12GraphicsPipelineObject&& rhs)
		: GraphicsPipelineObject(std::move(rhs)){
		m_pso.Swap(rhs.m_pso);
		m_rs.Swap(rhs.m_rs);
	}
	virtual ~DX12GraphicsPipelineObject() {};
public:
	ID3D12PipelineState* GetPSO() const { return m_pso.Get(); }
	ID3D12RootSignature* GetRootSignature() const { return m_rs.Get(); }
private:
	/**< 将成员变量还原为“最初状态" */
	void Reset();
private:
	SmartPTR<ID3D12PipelineState> m_pso;
	SmartPTR<ID3D12RootSignature> m_rs;
};

class DX12PipelineManager {
public:
	DX12PipelineManager(ID3D12Device* pDevice) : m_pDevice(pDevice) {}
public:
	/** 构造DX12需要的特定Pipeline object, 包括pso, rs等
	 * @param input 需要build的管线对象
	 * @param vs 和input关联的vertex shader对应的dx数据
	 * @param ps 和input关联的pixel shader对应的dx数据
	 * @return build失败返回空 */
	DX12GraphicsPipelineObject* Build(DX12GraphicsPipelineObject& input, const DX12Shader* vs, const DX12Shader* ps);
private:
	std::list<DX12GraphicsPipelineObject> m_graphicsPSOs;
	ID3D12Device* m_pDevice;
};

END_NAME_SPACE
