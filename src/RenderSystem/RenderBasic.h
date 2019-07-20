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

class RenderDevice {
public:
	enum DeviceState : uint8_t {
		DEVICE_STATE_UNINITIALIZE = 0u,
		DEVICE_STATE_RUNNING,
		DEVICE_STATE_PAUSE,
		DEVICE_STATE_SHUTDOWN
	};
public:
	RenderDevice(uint32_t width, uint32_t height) : ScreenWidth(width), ScreenHeight(height),
		m_nextBufferHandle(NUMBER_OF_SPECIAL_BUFFER_RESOURCE), m_nextTextureHandle(NUMBER_OF_SPECIAL_TEXTURE_RESOURCE),
		m_state(DEVICE_STATE_UNINITIALIZE) {}
	/** @remark 必须在子类调用完成后调用该函数修改状态 */
	virtual bool Initialize(std::string config) {
		m_state = DEVICE_STATE_RUNNING;
		return true;
	};
	void ShutDown() { m_state = DEVICE_STATE_SHUTDOWN; }
	DeviceState State() const { return m_state; }
	BufferDescriptor RequestBuffer(size_t size, ResourceStatus status, bool exclusive = false) thread_safe {
		if (exclusive) return BufferDescriptor(BufferHandle(static_cast<BufferHandle::ValueType>(m_nextBufferHandle++)), status, size);
		else return BufferDescriptor(BufferHandle::InvalidIndex(), status, size);
	}

	TextureDescriptor RequestTexture(uint32_t width, uint32_t height, ElementFormatType type,
		ResourceStatus status, bool exclusive = false) thread_safe {
		if (exclusive) return TextureDescriptor(TextureHandle(static_cast<TextureHandle::ValueType>(m_nextTextureHandle++)),
			status, width, height, type);
		else return TextureDescriptor(TextureHandle::InvalidIndex(), status, width, height, type);
	}
	TextureDescriptor RequestTexture(SpecialTextureResource specialResource) thread_safe {
		switch (specialResource) {
		case DEFAULT_BACK_BUFFER:
			return TextureDescriptor(TextureHandle(DEFAULT_BACK_BUFFER),
				{ RESOURCE_USAGE_RENDER_TARGET, RESOURCE_FLAG_STABLY }, ScreenWidth, ScreenHeight, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM);
			break;
		default:
			FLOG("Invalid special resource type!\n");
		}
		return TextureDescriptor::CreateInvalidDescriptor();
	}
	/** sampler只是描述性信息，暂不考虑是否临时资源 */
	SamplerDescriptor RequestSampler(FilterType filter, SamplerAddressMode u, SamplerAddressMode v,
		SamplerAddressMode w, const float(&color)[4] = { .0f, .0f, .0f, .0f }) thread_safe {
		return SamplerDescriptor(SamplerHandle(static_cast<SamplerHandle::ValueType>(m_nextSamplerHandle++)),
			filter, u, v, w, color);
	}
	/** 将task提交给RenderrDevice进行处理，调用后传入的task将被还原为默认状态 */
	void Submit(Task& task, uint64_t frameCount) thread_safe { 
		if (task.Commands.empty()) { 
			/** 不接受空的指令 */
			MLOG("Submited an empty task!, Maybe there is something wrong happend.");
			return;
		}
		task.Frame = frameCount;
		std::lock_guard<OptimisticLock> lg(m_taskQueueLock);
		m_taskQueue.push(std::move(task));
		task.Reset();
	}
	/** 允许额外的线程执行该函数，负责处理任务队列中的任务 */
	virtual void Process() = 0 thread_safe;
	const uint32_t ScreenWidth, ScreenHeight; /**< 屏幕的宽高，单位像素 */
protected:
	OptimisticLock m_taskQueueLock;
	std::queue<Task> m_taskQueue;
	DeviceState m_state;
private:
	std::atomic<BufferHandle::ValueType> m_nextBufferHandle;
	std::atomic<TextureHandle::ValueType> m_nextTextureHandle;
	std::atomic<SamplerHandle::ValueType> m_nextSamplerHandle;
};

class RenderBackend {
public:
	RenderBackend() : m_nextVertexAttributeHandle(0), m_nextProgramHandle(0) {}
	virtual ~RenderBackend() {}
	virtual bool Initialize() = 0;
	/** 创建依赖该API的设备 */
	virtual RenderDevice* CreateDevice(void* parameters) = 0;
	/** 创建一个程序，这个程序可能是compute类型(只有compute shader)，也可能是graphics类型
	 * @param shaderNames 该program种使用的各个shader的名称
	 * @param opts 程序中的一些操作进行设置
	 * @param vtxAttDesc 程序使用到的顶点属性，必须覆盖shader中所有的顶点属性
	 * @param usedIndex 是否使用索引，仅对graphics程序有效 
	 * @return 返回程序的描述器，包含程序关键信息 */
	virtual ProgramDescriptor RequestProgram(const ShaderNames& shaderNames, VertexAttributeHandle va_handle,
		bool usedIndex, RasterizeOptions rasterization, OutputStageOptions outputStage) = 0 thread_safe;
	virtual bool isInitialized() const { return false; }
	/** 注册顶点结构描述，以便重复使用 */
	virtual VertexAttributeHandle RegisterVertexAttributeDescs(const VertexAttributeDescs& descs) = 0;
protected:
	 OptimisticLock m_shaderAnalysationLock;
	std::map<std::string, std::unique_ptr<std::mutex> > m_mapOfShaderAnalysationLock;

	std::atomic<ProgramHandle::ValueType> m_nextProgramHandle;
	std::atomic<VertexAttributeHandle::ValueType> m_nextVertexAttributeHandle;
};

END_NAME_SPACE
