#pragma once

#include "RenderSystemConfig.h"
#include <limits>
#include <functional>
BEG_NAME_SPACE

struct Task;

class RenderDevice {
public:
	virtual bool Initialize(std::string config) = 0;
	virtual BufferHandle RequestBuffer(size_t size, ResourceUsage usage, ResourceFlag flag, bool manual = true) = 0 thread_safe;

	/** Note 应该允许多个线程同时提交Task */
	virtual std::function<void()> Process(Task&) = 0 thread_safe;
};

class RenderBackend {
public:
	virtual ~RenderBackend() {}
	virtual bool Initialize() = 0;
	/** 创建依赖该API的设备 */
	virtual RenderDevice* CreateDevice(void* parameters) = 0;
	virtual bool isInitialized() const { return false; }
};

END_NAME_SPACE
