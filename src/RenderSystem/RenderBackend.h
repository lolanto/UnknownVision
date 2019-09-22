#pragma once

#include "../Utility/OptimisticLock/OptimisticLock.hpp"
#include "../Utility/InfoLog/InfoLog.h"
#include "../UVType.h"
#include "../UVConfig.h"
#include "Pipeline.h"
#include <atomic>
#include <mutex>
#include <string>
#include <memory>

BEG_NAME_SPACE

class RenderDevice;
class BasicShader;
/** 负责创建设备以及提供设备无关的功能 */
class RenderBackend {
public:
	RenderBackend() {}
	virtual ~RenderBackend() {}
	virtual bool Initialize() = 0;
	virtual bool isInitialized() const { return false; }
public:
	/** 创建依赖该API的设备 */
	virtual RenderDevice* CreateDevice(void* parameters) { return nullptr; }
	/** 初始化Shader对象，主要是配置ShaderHandle */
	virtual bool InitializeShaderObject(BasicShader* shader) { return false; }
	/** 构造GraphicsPipeline */
	virtual GraphicsPipelineObject* BuildGraphicsPipelineObject(
		VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rastOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList
	) {
		return nullptr;
	}
};

END_NAME_SPACE
