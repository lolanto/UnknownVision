#pragma once
#include "DX12Config.h"
#include "DX12Pipeline.h"
#include "DX12Shader.h"
#include "../RenderSystem/RenderBackend.h"
#include <assert.h>
BEG_NAME_SPACE
class DX12RenderDevice;

class DX12RenderBackend : public RenderBackend {
public:
	~DX12RenderBackend() {
		for (auto devPtr : m_devices)
			delete devPtr;
	}
public:
	bool Initialize() final;
	bool isInitialized() const final { return m_isInitialized; }
public:
	RenderDevice* CreateDevice(void* parameters) final;
	/** 完成Shader对象的初始化工作，包括编译，反射信息提取 */
	bool InitializeShaderObject(BasicShader* shader) final;

	/** 构造GraphicsPipeline */
	virtual GraphicsPipelineObject* BuildGraphicsPipelineObject(
		VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rastOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList
	) final;

private:
	bool m_isInitialized = false;
	SmartPTR<IDXGIFactory6> m_factory;
	std::vector<DX12RenderDevice*> m_devices;
	DX12ShaderManager m_shaderManager; /**< 负责创建，存储和检索Shader Object */
	DX12PipelineManager m_pipelineManager; /**< 负责创建，存储和检索Pipeline Object */
};

END_NAME_SPACE
