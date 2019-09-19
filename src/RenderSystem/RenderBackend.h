#pragma once

#include "../Utility/OptimisticLock/OptimisticLock.hpp"
#include "../Utility/InfoLog/InfoLog.h"
#include "../UVType.h"
#include "../UVConfig.h"
#include "RenderDescriptor.h"
#include "Task.h"
#include <atomic>
#include <mutex>
#include <queue>
#include <string>

BEG_NAME_SPACE

class RenderDevice;
class BasicShader;

class RenderBackend {
public:
	RenderBackend() : m_nextVertexAttributeHandle(0) {}
	virtual ~RenderBackend() {}
	virtual bool Initialize() = 0;
	/** 创建依赖该API的设备 */
	virtual RenderDevice* CreateDevice(void* parameters) = 0;
	virtual bool isInitialized() const { return false; }
	/** 注册顶点结构描述，以便重复使用 */
	virtual VertexAttributeHandle RegisterVertexAttributeDescs(const VertexAttributeDescs& descs) = 0;
	virtual bool InitializeShaderObject(BasicShader* shader) { return false; }
protected:
	std::atomic<VertexAttributeHandle::ValueType> m_nextVertexAttributeHandle;
};

END_NAME_SPACE
