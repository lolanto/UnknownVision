#pragma once
#include "DX12Config.h"
#include "DX12Shader.h"
#include "DX12ShaderResource.h"
#include "../RenderSystem/Shader.h"
#include "../RenderSystem/Pipeline.h"

#include <map>
#include <string>

BEG_NAME_SPACE

/** 用来描述参数名到Descriptor的关系 */
struct DescriptorSetting {
	uint16_t tableIndex; /**< 当前参数所在的descriptor table的索引 */
	uint16_t descriptorOffset; /**< 当前参数在table内的偏移 */
};

struct DescriptorTableSetting {
	D3D12_GPU_DESCRIPTOR_HANDLE cachedHandle; /**< 当前缓存的view在GPU Descriptor Heap中的handle */
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	uint8_t numOfDescriptors;
	uint8_t offsetFromBegin; /**< 相比较第一个Descriptor的偏移，中间差距多少个Descriptor */
};

class DX12GraphicsPipelineObject : public GraphicsPipelineObject {
	friend class DX12PipelineManager;
	union View {
		DX12ShaderResourceView* pSRV;
		DX12ConstantBufferView* pCBV;
		DX12UnorderAccessView* pUAV;
	};
public:
	DX12GraphicsPipelineObject(VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rasOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList)
		: GraphicsPipelineObject(vs, ps, rasOpt, outputOpt, vtxAttribList) {}
private:
	/**< 将成员变量还原为“最初状态" */
	void Reset();
private:
	SmartPTR<ID3D12PipelineState> m_pso;
	SmartPTR<ID3D12RootSignature> m_rs;
	std::map<std::string, DescriptorSetting> m_NameToDescriptorSettings;
	std::vector<DescriptorTableSetting> m_DescriptorTableSettings;
	std::vector<View> m_cachedViews; /**< 当前缓存的view设置 */
};

class DX12PipelineManager {
public:
	DX12PipelineManager(ID3D12Device* pDevice) : m_pDevice(pDevice) {}
public:
	/** 构造DX12需要的特定Pipeline object, 包括pso, rs等
	 * @param input 需要build的管线对象
	 * @param vs 和input关联的vertex shader对应的dx数据
	 * @param ps 和input关联的pixel shader对应的dx数据
	 * @return build成功返回true否则返回false */
	bool Build(DX12GraphicsPipelineObject& input, const DX12Shader* vs, const DX12Shader* ps);
private:
	std::list<DX12GraphicsPipelineObject> m_graphicsPSOs;
	ID3D12Device* m_pDevice;
};

END_NAME_SPACE
