#pragma once
#include "../UVType.h"
#include "CommandUnit.h"
#include <atomic>
#include <string>

BEG_NAME_SPACE
/** 负责管理该Device对应的GPU
 * 包括提供设备的状态查询和设置
 * 与设备关联的资源的创建和删除(不直接创建，但需要作为参数传入到资源的创建函数中) */
class RenderDevice {
public:
	enum DeviceState : uint8_t {
		DEVICE_STATE_UNINITIALIZE = 0u,
		DEVICE_STATE_RUNNING,
		DEVICE_STATE_PAUSE,
		DEVICE_STATE_SHUTDOWN
	};
public:
	/** 初始化相关函数和状态查询函数 */
	RenderDevice(uint32_t width, uint32_t height) : ScreenWidth(width), ScreenHeight(height),
		m_state(DEVICE_STATE_UNINITIALIZE) {}
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
public:
	virtual CommandUnit* RequestCommandUnit(COMMAND_UNIT_TYPE type) = 0;
	/** 释放指令单元，所有已录制未执行的指令都将丢失 */
	virtual void FreeCommandUnit(CommandUnit** cmdUnit) = 0;
	virtual void UpdatePerFrame() {}

	/** 构造GraphicsPipeline */
	virtual GraphicsPipelineObject* BuildGraphicsPipelineObject(
		VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rastOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList
	) {
		return nullptr;
	}

	GPUResource* BackBuffer() { return nullptr; }

protected:
	DeviceState m_state;
};

END_NAME_SPACE
