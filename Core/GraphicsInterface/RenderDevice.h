#pragma once
#include "../UVType.h"
#include "CommandUnit.h"
#include "Pipeline.h"
#include <atomic>
#include <string>

BEG_NAME_SPACE
class BindingBoard;
class Image;

/** 对存储于系统内存的图片数据的描述
 * 记录了图片数据的对齐信息，用于向Texture资源拷贝数据 */
struct ImageDesc {
	size_t width, height, depth; /**< 图片的长宽高，二维图片的depth = 1 */
	size_t rowPitch; /**< 图片一行的字节数量，需要考虑对齐 */
	size_t slicePitch; /**< 图片一整张的大小，需要考虑对齐 */
	void* data; /**< 指向图片数据的缓冲区指针 */
};

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
	RenderDevice(uint32_t width, uint32_t height, size_t node) : ScreenWidth(width), ScreenHeight(height),
		NodeMask(node), m_state(DEVICE_STATE_UNINITIALIZE) {}
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
	const size_t NodeMask; /**< 设备节点 */
public:
	/** 等待某一CmdUnit提交的任务执行完成 */
	virtual void WaitForCommandExecutionComplete(size_t fenceValue, COMMAND_UNIT_TYPE type) = 0;
	/** 查询某一CmdUnit提交的任务是否已经完成 */
	virtual bool QueryCommandExecutionState(size_t fenceValue, COMMAND_UNIT_TYPE type) = 0;
	/** 请求与该设备绑定的指令单元
	 * @param type 指令单元类型
	 * @return 返回创建成功的指令单元指针，失败返回nullptr */
	virtual CommandUnit* RequestCommandUnit(COMMAND_UNIT_TYPE type) = 0;
	/** 释放指令单元，所有已录制未执行的指令都将丢失 */
	virtual void FreeCommandUnit(CommandUnit** cmdUnit) = 0;
	/** 每一帧都要执行的任务写于此 */
	virtual void UpdatePerFrame() {}

	/** 构造GraphicsPipeline
	 * @remark PSO归Device管理，请勿手动释放返回的指针!*/
	virtual GraphicsPipelineObject* BuildGraphicsPipelineObject(
		VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rastOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList
	) {
		return nullptr;
	}
	/** 返回当前正在使用的backbuffer指针
	 * -1 表示当前帧可以写入的backbuffer */
	virtual Texture2D* BackBuffer(size_t idx = SIZE_MAX) { return nullptr; }
	/** 返回当前正在使用的depth stencil buffer指针 
	 * -1 表示当前帧可以写入的depth stencil buffer */
	virtual Texture2D* DepthStencilBuffer(size_t idx = SIZE_MAX) { return nullptr; }
	/** 创建一维缓冲资源
	 * @param capacity 缓冲容纳元素的数量
	 * @param elementStride 一个元素的字节大小
	 * @param status 资源的状态描述，详见具体定义
	 * @return 返回创建成功后的资源，创建失败返回null */
	virtual Buffer* CreateBuffer(size_t capacity, size_t elementStride, ResourceStatus status) { return nullptr; }
	/** 创建二维纹理资源
	 * @param width 二维纹理的宽度像素数量
	 * @param height 二维纹理的高度像素数量
	 * @param miplevels mipmap层级
	 * @param arrSize 2D纹理数组大小，单张纹理为1即可
	 * @param format 纹理像素格式，详见类型定义
	 * @param status 资源状态描述，详见具体定义 
	 * @return 返回创建成功后的资源指针，创建失败返回null */
	virtual Texture2D* CreateTexture2D(size_t width, size_t height, size_t miplevels, size_t arrSize, ElementFormatType format, ResourceStatus status) { return nullptr; }
	/** 向缓冲中写入数据
	 * @param pSrc 指向存储写入数据缓冲的指针
	 * @param pDest 指向需要写入数据的缓冲资源 
	 * @param srcSize 写入数据的字节大小
	 * @param destOffset 写入时从pDest起点偏移的字节数量
	 * @param cmdUnit 执行写入需要使用的指令单元
	 * @return 返回写入是否成功
	 * @remark 需要“假设”该调用会使cmdUnit之前录制的所有指令都被触发，同时线程等待指令执行完成后才返回
	 * 同时需要保证目标缓冲的状态必须为Copy_dest*/
	virtual bool WriteToBuffer(void* pSrc, Buffer* pDest, size_t srcSize, size_t destOffset, CommandUnit* cmdUnit) { return false; }
	/** 向二维纹理中写入图片
	 * @param srcDesc 描述写入图片的内存格式和地址，每个图片代表一级miplevel
	 * @param pDest 指向被写入的纹理资源
	 * @param cmdUnit 负责录制写入指令的指令单元
	 * @return 返回写入是否成功
	 * @remark 需要“假设”该调用会使cmdUnit之前录制的所有指令被触发，同时线程等待指令执行完成后才返回
	 * 同时需要保证目标纹理状态必须为Copy_dest */
	virtual bool WriteToTexture2D(const std::vector<ImageDesc>& srcDesc, Texture2D* pDest, CommandUnit* cmdUnit) { return false; }
	/** 向二维纹理数组中写入图片
	 * @param srcDesc 描述写入图片的内存格式和地址，每个vector<ImageDesc>代表一个纹理的mipLevels
	 * @param pDest 指向被写入的纹理资源
	 * @param cmdUnit 负责录制写入指令的指令单元
	 * @return 返回写入是否成功
	 * @remark 需要“假设”该调用会cmdUnit之前录制的所有指令被触发，同时线程等待指令执行完成后才返回
	 *					需要保证 srcDesc中每个子数组的长度相同 */
	virtual bool WriteToTexture2DArr(const std::vector<std::vector<ImageDesc>>& srcDesc, Texture2D* pDest, CommandUnit* cmdUnit) { return false; }
	/** 从二维纹理中读取数据
	 * @param output 返回输出的纹理数据，大小，格式均需要从pDest中另外获取
	 * @param pSrc 需要读取的目标纹理
	 * @param cmdUnit 负责执行读取指令的指令单元
	 * @return 返回读取是否成功
	 * @remark cmdUnit之前录制的所有指令将会被执行，且线程会等待指令读取完成后才返回
	 *		需要用户确保函数调用时pSrc已经绘制完毕 */
	virtual bool ReadFromTexture2D(std::vector<uint8_t>& output, Texture2D* pSrc, CommandUnit* cmdUnit) { return false; }
	/** 请求一个bindingBoard
	 * @param numOfSlots bindingBoard容纳的参数数量
	 * @param type 使用bindingBoard的指令单元类型 
	 * @return 创建成功返回指向bindingBoard的指针，失败返回nullptr */
	virtual BindingBoard* RequestBindingBoard(size_t numOfSlots, COMMAND_UNIT_TYPE type) { return nullptr; }
	/** 实例化bindingBoard
	 * @param pboard 指向需要实例化的bindingBoard
	 * @return 返回实例化成功与否
	 * @remark 实例化后的bindingBoard将占用一定量GPU资源，务必及时释放 */
	virtual bool InstanceBindingBoard(BindingBoard* pboard) { return false; }
protected:
	DeviceState m_state;
};

END_NAME_SPACE
