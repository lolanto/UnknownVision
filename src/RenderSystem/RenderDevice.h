#pragma once
#include "../UVType.h"
#include "CommandUnit.h"
#include <atomic>
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
		m_state(DEVICE_STATE_UNINITIALIZE), m_nextProgramHandle(0) {}
	virtual ~RenderDevice() = default;
	/** @remark 必须在子类调用完成后调用该函数修改状态 */
	virtual bool Initialize(std::string config) {
		m_state = DEVICE_STATE_RUNNING;
		return true;
	};
	void ShutDown() { m_state = DEVICE_STATE_SHUTDOWN; }
	DeviceState State() const { return m_state; }
	virtual bool Present() = 0; /**< 将当前backbuffer内容进行换页 */

	const uint32_t ScreenWidth, ScreenHeight; /**< 屏幕的宽高，单位像素 */
/** 资源请求类的操作 */
public:
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

	virtual bool RevertResource(BufferHandle handle) = 0 thread_safe;
	virtual bool RevertResource(TextureHandle handle) = 0 thread_safe;

	virtual CommandUnit& RequestCommandUnit(COMMAND_UNIT_TYPE type) = 0;

	virtual SpecialTextureResource CurrentBackBufferHandle() = 0;

protected:
	DeviceState m_state;
	std::atomic<ProgramHandle::ValueType> m_nextProgramHandle;
private:
	std::atomic<BufferHandle::ValueType> m_nextBufferHandle;
	std::atomic<TextureHandle::ValueType> m_nextTextureHandle;
	std::atomic<SamplerHandle::ValueType> m_nextSamplerHandle;
};

END_NAME_SPACE
