#pragma once
#include "../UVType.h"
#include <vector>
#include <array>
BEG_NAME_SPACE

class RenderDevice;
class GraphicsPipelineObject;
class GPUResource;
class BindingBoard;
struct ViewportDesc;
struct ScissorRectDesc;
class GPUBuffer;

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

	/** 配置bindingBoards，相当于配置一系列的参数集合
	 * @param slot  需要将参数绑定到哪一个位置上
	 * @param board 需要往目标slot上绑定的参数集合 
	 * @desc: 以防之后忘记，我这里写详细一点。这是DX12的一个概念：
	 * 不同Shader共同构建PSO，这些Shader里面用到的所有参数，最终就反应到PSO用到的参数上，也就是RootSignature
	 * 一个RootSignature上可以绑多个RootParameter
	 * 一个RootParameter可以是常数，也可以是一个或者一系列Descriptor(Table)
	 * Descriptor对应一个Shader参数的声明，即它是c0，t0这些
	 * BindingBoard在这里就对应一个RootParameter。一般而言，一个Shader一个RootParameter，里面再用DescriptorTable的方式声明参数排布方式就够了
	 * 在这个基础上假如是VS + PS的组合，那么slot 0对应的是VS的RootParameter(Descriptor Table)，slot 1对应的是PS的RootParameter(Descriptor Table)
	 */
	virtual void SetBindingBoard(size_t slot, BindingBoard* board) = 0;
	/** 绑定顶点缓冲 */
	virtual void BindVertexBuffers(size_t startSlot, size_t numberOfBuffers, GPUBuffer** ppBuffers) = 0;
	/** 绑定索引缓冲 */
	virtual void BindIndexBuffer(GPUBuffer* pBuffer) = 0;
	/** 绑定渲染目标 */
	virtual void BindRenderTargets(GPUResource** ppRenderTargets, size_t numRenderTargets, GPUResource* pDepthStencil) = 0;
	/** 发起Draw指令 */
	virtual void Draw(size_t startOfIndex, size_t indexCount, size_t startOfVertex) = 0;
	/** 修改资源状态 */
	virtual void TransferState(GPUResource* pResource, ResourceStates newState) = 0;
	/** 绑定viewport */
	virtual void BindViewports(size_t size, const ViewportDesc* viewports) = 0;
	/** 绑定scissor rect */
	virtual void BindScissorRects(size_t size, const ScissorRectDesc* scissorRects) = 0;
	/** 清空渲染目标 */
	virtual void ClearRenderTarget(GPUResource* renderTarget, const float* clearColor) = 0;
	/** 清空深度缓冲 */
	virtual void ClearDepthStencilBuffer(GPUResource* ds, float depth = 1.0f, uint8_t stencil = 0) = 0;
};

END_NAME_SPACE
