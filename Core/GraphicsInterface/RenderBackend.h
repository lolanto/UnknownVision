#pragma once

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
/** 负责创建设备以及提供设备无关的功能
 * Note: 保证Backend必须是单例 */
class RenderBackend : public Standalone {
public:
	static RenderBackend* Get();
protected:
	RenderBackend() {}
public:
	virtual ~RenderBackend() {}
	virtual bool Initialize() = 0;
	virtual bool isInitialized() const { return false; }
public:
	/** 创建依赖某一API的设备，API类型在编译时由宏确定
	 * @param parameters 指向存储创建设备所需要参数的缓冲，参数类型与API相关，具体内容见定义 */
	virtual RenderDevice* CreateDevice(void* parameters) { return nullptr; }
	/** 获得某一硬件节点的设备 */
	virtual RenderDevice* GetDevice(size_t node) { return nullptr; }
	/** 初始化Shader对象，主要是配置ShaderHandle */
	virtual bool InitializeShaderObject(BasicShader* shader) { return false; }
};

END_NAME_SPACE
