#pragma once
#include "../DX12Config.h"
#include "../DX12Helpers.h"
#include "../DX12DescriptorHeap.h"
#include "../../RenderSystem/CommandUnit.h"

#include <vector>
#include <queue>
#include <mutex>

BEG_NAME_SPACE

class DX12CommandListManager;
class DX12RenderDevice;
class DX12CommandQueue;

/** commandUnit是由Device按需分配的，执行完成后由Device回收，
 * 需要时由Device重新生成 */
class DX12CommandUnit : public CommandUnit {
	friend class DX12RenderDevice;
	friend class DX12CommandUnitManager;
public:
	/** 公开的接口 */
	RenderDevice* GetDevice() override;
	/** 提交当前录制的指令 */
	virtual size_t Flush(bool bWaitForCompletion = false) final;
	virtual void BindPipeline(GraphicsPipelineObject* gpo) final;
	/** 绑定顶点缓冲 */
	virtual void BindVertexBuffers(size_t startSlot, size_t numberOfBuffers, GPUResource** ppBuffers) final;
	/** 绑定索引缓冲 */
	virtual void BindIndexBuffer(GPUResource* pBuffer) final;
	/** 绑定RTV */
	virtual void BindRenderTargets(GPUResource** ppRenderTargets, size_t numRenderTargets, GPUResource* pDepthStencil) final;
	/** 发起Draw指令 */
	virtual void Draw(size_t startOfIndex, size_t indexCount, size_t startOfVertex) final;
	/** 修改资源状态 */
	virtual void TransferState(GPUResource* pResource, ResourceStates newState) final;
	/** 翻转swapchain */
	virtual void Present() final;
	/** 绑定viewport */
	virtual void BindViewports(size_t size, ViewPort* viewports) final;
	/** 绑定scissor rect */
	virtual void BindScissorRects(size_t size, ScissorRect* scissorRects) final;
	/** 清空渲染目标 */
	virtual void ClearRenderTarget(GPUResource* renderTarget, const float* clearColor) final;
public:
	/** DX12辅助接口 */
	DX12CommandUnit(COMMAND_UNIT_TYPE type = static_cast<COMMAND_UNIT_TYPE>(0xff))
		: CommandUnit(type), m_pDevice(nullptr), m_graphicsCmdList(nullptr), m_pAllocator(nullptr) {}
	~DX12CommandUnit() {}
	/** 初始化device, cmdList以及allocator的指向
	 * @remark 一旦device和cmdList确定后将不再允许修改*/
	bool Initalize(DX12RenderDevice* pDevice, SmartPTR<ID3D12GraphicsCommandList> graphicsCmdList, ID3D12CommandAllocator* allocator);
	/** 重置Command Allocator，前提是已经初始化完成 */
	bool Reset(ID3D12CommandAllocator* allocator);

	/** 在GPU上拷贝两个缓冲 */
	void CopyBetweenBuffer(ID3D12Resource* destBuffer, ID3D12Resource* srcBuffer);
	/** 创建一个临时缓冲，用来作为中间缓冲进行数据传输，临时缓冲在Command执行完后被删除 */
	ID3D12Resource* TransientBuffer(size_t bufferSizeInByte, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD);
private:
	/** 删除当前指令引用的临时资源的函数 */
	std::function<bool()> cleanupFunction(size_t fenceValue, DX12CommandQueue& queue);
private:
	DX12RenderDevice* m_pDevice;
	/** TODO: 暂时只支持一个graphic command list */
	SmartPTR<ID3D12GraphicsCommandList> m_graphicsCmdList;
	ID3D12CommandAllocator* m_pAllocator;

	std::vector<ID3D12Resource*> m_transientResources; /**< 指令执行完毕后删除的临时资源 */
	TransientDX12DescriptorHeap m_transientDescriptorHeapForGPU; /**< 临时的Descriptor heap */
	TransientDX12DescriptorHeap m_transientDescriptorHeapForRTV;
	TransientDX12DescriptorHeap m_transientDescriptorHeapForDSV;
};

class DX12CommandUnitManager {
public:
	DX12CommandUnitManager()
		: m_pDevice(nullptr), m_pCmdListManager(nullptr) {}
public:
	bool Create(DX12RenderDevice* pDevice);
	DX12CommandUnit* AllocateCommandUnit(COMMAND_UNIT_TYPE type);
	void FreeCommandUnit(DX12CommandUnit** unit);
private:
	DX12RenderDevice* m_pDevice;
	DX12CommandListManager* m_pCmdListManager;

	std::mutex m_freeCmdUnitMutex;
	std::mutex m_cmdUnitsMutex;
	std::queue<DX12CommandUnit*> m_freeCmdUnits[NUMBER_OF_COMMAND_UNIT_TYPE];
	std::vector<DX12CommandUnit> m_cmdUnits[NUMBER_OF_COMMAND_UNIT_TYPE];
};

END_NAME_SPACE
