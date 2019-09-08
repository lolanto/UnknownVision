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

class CommandListener {

};

/** 资源应用类指令的接口集合 */
class CommandUnit {
public:
	enum VariableType {
		
	};
	virtual ~CommandUnit() = default;
public:
	virtual bool Active() = 0;
	virtual bool Fetch() = 0;
	virtual bool FetchAndPresent() = 0;
	virtual bool Wait() = 0;
	virtual bool Reset() = 0;
	/** 仅针对CPU可写的缓冲 */
	virtual bool UpdateBufferWithSysMem(BufferHandle dest, void* src, size_t size) = 0;
	/** 仅针对可读回的，以及CPU可读的缓冲 */
	virtual bool ReadBackToSysMem(BufferHandle src, void* dest, size_t size) = 0;
	virtual bool CopyBetweenGPUBuffer(BufferHandle src, BufferHandle dest, size_t srcOffset, size_t destOffset, size_t size) = 0;
	virtual bool TransferState(BufferHandle buf, ResourceStates newState) = 0;
	virtual bool TransferState(TextureHandle tex, ResourceStates newState) = 0;
	//virtual bool UseProgram(ProgramHandle program) = 0;
	virtual bool BindVariable(VariableType type, ) = 0;
	//virtual bool BindVertexBuffers(const std::vector<BufferHandle>& vtxBuffers) = 0;
	//virtual bool BindIndexBuffer(BufferHandle buf) = 0;
	virtual bool BindRenderTargetsAndDepthStencilBuffer(const std::vector<TextureHandle>& renderTargets, TextureHandle depthStencil) = 0;
	virtual bool ClearRenderTarget(TextureHandle renderTarget, const std::array<float, 4>& color) = 0;
};

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
		m_state(DEVICE_STATE_UNINITIALIZE), m_nextProgramHandle(0) {}
	virtual ~RenderDevice() = default;
	/** @remark 必须在子类调用完成后调用该函数修改状态 */
	virtual bool Initialize(std::string config) {
		m_state = DEVICE_STATE_RUNNING;
		return true;
	};
	void ShutDown() { m_state = DEVICE_STATE_SHUTDOWN; }
	DeviceState State() const { return m_state; }

	/** sampler只是描述性信息，暂不考虑是否临时资源 */
	//SamplerDescriptor RequestSampler(FilterType filter, SamplerAddressMode u, SamplerAddressMode v,
	//	SamplerAddressMode w, const float(&color)[4] = { .0f, .0f, .0f, .0f }) thread_safe {
	//	return SamplerDescriptor(SamplerHandle(static_cast<SamplerHandle::ValueType>(m_nextSamplerHandle++)),
	//		filter, u, v, w, color);
	//}

	/** 创建一个程序，这个程序可能是compute类型(只有compute shader)，也可能是graphics类型
	* @param shaderNames 该program种使用的各个shader的名称
	* @param opts 程序中的一些操作进行设置
	* @param vtxAttDesc 程序使用到的顶点属性，必须覆盖shader中所有的顶点属性
	* @param usedIndex 是否使用索引，仅对graphics程序有效
	* @param rasterization 光栅过程的设置
	* @param outputStage 输出过程的设置
	* @param staticSamplers 静态采样器的设置，设置格式为: "name of the static sampler", samplerHandle
	* @return 返回程序的描述器，包含程序关键信息 */
	virtual ProgramDescriptor RequestProgram(const ShaderNames& shaderNames, VertexAttributeHandle va_handle,
		bool usedIndex, RasterizeOptions rasterization, OutputStageOptions outputStage,
		const std::map<std::string, const SamplerDescriptor&>& staticSamplers = {}) = 0 thread_safe;

	virtual ProgramHandle RequestProgram2(const ShaderNames& shaderNames, VertexAttributeHandle va_handle,
		bool usedIndex, RasterizeOptions rasterization, OutputStageOptions outputStage,
		const std::map<std::string, const SamplerDescriptor&>& staticSamplers = {}) = 0 thread_safe;

	const uint32_t ScreenWidth, ScreenHeight; /**< 屏幕的宽高，单位像素 */

	/** TODO: 暂时只能请求一个二维纹理 */
	virtual TextureHandle RequestTexture(uint32_t width, uint32_t height, ElementFormatType type,
		ResourceStatus status) thread_safe {
		return TextureHandle(static_cast<TextureHandle::ValueType>(m_nextTextureHandle++));
	}

	virtual TextureHandle RequestTexture(SpecialTextureResource specialResource) thread_safe {
		return TextureHandle(static_cast<uint8_t>(specialResource));
	}

	/** 假如缓冲是作为顶点或者索引缓冲，则stride表明缓冲中一个元素的大小 */
	virtual BufferHandle RequestBuffer(size_t size, ResourceStatus status, size_t stride = 0) thread_safe {
		return BufferHandle(static_cast<BufferHandle::ValueType>(m_nextBufferHandle++));
	}

	virtual bool RevertResource(BufferHandle handle)= 0 thread_safe;
	virtual bool RevertResource(TextureHandle handle) = 0 thread_safe;

	virtual CommandUnit& RequestCommandUnit(COMMAND_UNIT_TYPE type) = 0;

	virtual bool Present() = 0;

	virtual SpecialTextureResource CurrentBackBufferHandle() = 0;

protected:
	DeviceState m_state;
	std::atomic<ProgramHandle::ValueType> m_nextProgramHandle;
private:
	std::atomic<BufferHandle::ValueType> m_nextBufferHandle;
	std::atomic<TextureHandle::ValueType> m_nextTextureHandle;
	std::atomic<SamplerHandle::ValueType> m_nextSamplerHandle;
};

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
protected:
	std::atomic<VertexAttributeHandle::ValueType> m_nextVertexAttributeHandle;
};

END_NAME_SPACE
