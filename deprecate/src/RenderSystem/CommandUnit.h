#pragma once
#include "../UVType.h"
#include <vector>
#include <array>
BEG_NAME_SPACE

class RenderDevice;
class GraphicsPipelineObject;
class GPUResource;
class BindingBoard;
struct ViewPort;
struct ScissorRect;

/** 指令队列的编辑器，按顺序向底层的CommandList写入指令 */
class CommandUnit {
public:
	CommandUnit(COMMAND_UNIT_TYPE type = static_cast<COMMAND_UNIT_TYPE>(0xff)) : CommandUnitType(type) {}
public:
	virtual RenderDevice* GetDevice() { return nullptr; }
	virtual ~CommandUnit() = default;
	const COMMAND_UNIT_TYPE CommandUnitType;
public:
	/** 提交当前录制的指令 */
	virtual size_t Flush(bool bWaitForCompletion = false) = 0;
	/** 绑定pipeline */
	virtual void BindPipeline(GraphicsPipelineObject* gpo) = 0;
	/** 配置bindingBoards */
	virtual void SetBindingBoard(size_t slot, BindingBoard* board) = 0;
	/** 绑定顶点缓冲 */
	virtual void BindVertexBuffers(size_t startSlot, size_t numberOfBuffers, Buffer** ppBuffers) = 0;
	/** 绑定索引缓冲 */
	virtual void BindIndexBuffer(Buffer* pBuffer) = 0;
	/** 绑定渲染目标 */
	virtual void BindRenderTargets(GPUResource** ppRenderTargets, size_t numRenderTargets, GPUResource* pDepthStencil) = 0;
	/** 发起Draw指令 */
	virtual void Draw(size_t startOfIndex, size_t indexCount, size_t startOfVertex) = 0;
	/** 修改资源状态 */
	virtual void TransferState(GPUResource* pResource, ResourceStates newState) = 0;
	/** 绑定viewport */
	virtual void BindViewports(size_t size, ViewPort* viewports) = 0;
	/** 绑定scissor rect */
	virtual void BindScissorRects(size_t size, ScissorRect* scissorRects) = 0;
	/** 清空渲染目标 */
	virtual void ClearRenderTarget(GPUResource* renderTarget, const float* clearColor) = 0;
};

END_NAME_SPACE
