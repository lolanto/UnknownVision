#pragma once
#include "DX12Config.h"
#include "DX12Helpers.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommandAllocatorPool.h"
#include "../GraphicsInterface/CommandUnit.h"

#include <vector>
#include <list>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <functional>

BEG_NAME_SPACE

class DX12RenderDevice;
class DX12BindingBoard;
class DX12CommandUnitManager;
class Buffer;

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
	/** 配置bindingBoards */
	virtual void SetBindingBoard(size_t slot, BindingBoard* board) final;
	/** 绑定顶点缓冲 */
	virtual void BindVertexBuffers(size_t startSlot, size_t numberOfBuffers, Buffer** ppBuffers) final;
	/** 绑定索引缓冲 */
	virtual void BindIndexBuffer(Buffer* pBuffer) final;
	/** 绑定RTV */
	virtual void BindRenderTargets(GPUResource** ppRenderTargets, size_t numRenderTargets, GPUResource* pDepthStencil) final;
	/** 发起Draw指令 */
	virtual void Draw(size_t startOfIndex, size_t indexCount, size_t startOfVertex) final;
	/** 修改资源状态 */
	virtual void TransferState(GPUResource* pResource, ResourceStates newState) final;
	/** 绑定viewport */
	virtual void BindViewports(size_t size, ViewPort* viewports) final;
	/** 绑定scissor rect */
	virtual void BindScissorRects(size_t size, ScissorRect* scissorRects) final;
	/** 清空渲染目标 */
	virtual void ClearRenderTarget(GPUResource* renderTarget, const float* clearColor) final;
	/** 清空depth stencil buffer */
	virtual void ClearDepthStencilBuffer(GPUResource* ds, float depth = 1.0f, uint8_t stencil = 0) final;
public:
	/** DX12辅助接口 */
	DX12CommandUnit(COMMAND_UNIT_TYPE type = static_cast<COMMAND_UNIT_TYPE>(0xff))
		: CommandUnit(type), m_pMgr(nullptr), m_graphicsCmdList(nullptr), m_pAllocator(nullptr) {}
	~DX12CommandUnit() {}
	/** 初始化device, cmdList以及allocator的指向
	 * @remark 一旦device和cmdList确定后将不再允许修改*/
	bool Initalize(DX12CommandUnitManager* pMgr,
		SmartPTR<ID3D12GraphicsCommandList> graphicsCmdList, ID3D12CommandAllocator* allocator, DX12RenderDevice* pDevice);
	/** 重置Command Allocator，前提是已经初始化完成 */
	bool Reset(ID3D12CommandAllocator* allocator);
	/** 在GPU上拷贝两个缓冲 */
	void CopyBetweenBuffer(ID3D12Resource* destBuffer, ID3D12Resource* srcBuffer);
	/** 创建一个临时缓冲，用来作为中间缓冲进行数据传输，临时缓冲在Command执行完后被删除 */
	//ID3D12Resource* TransientBuffer(size_t bufferSizeInByte, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD);
private:
	void loopValidBindingBoard(std::function<void(std::pair<size_t, DX12BindingBoard*>)> func);
private:
	size_t m_lastFenceValue;
	DX12CommandUnitManager* m_pMgr;
	DX12RenderDevice* m_pDevice;
	/** TODO: 暂时只支持一个graphic command list */
	SmartPTR<ID3D12GraphicsCommandList> m_graphicsCmdList;
	ID3D12CommandAllocator* m_pAllocator;
	std::unordered_map<size_t, BindingBoard*> m_bindingSlotToBindingBoards;
};


class DX12CommandUnitManager {
public:
	DX12CommandUnitManager(COMMAND_UNIT_TYPE type) : Type(type), m_fenceEventHandle(nullptr),
		m_allocatorPool(CommandUnitTypeToDX12CommandListType(type)) {}
	~DX12CommandUnitManager();
	bool Create(DX12RenderDevice* pDevice, SmartPTR<ID3D12CommandQueue> queue = nullptr);
	DX12CommandUnit* AllocateCommandUnit();
	void FreeCommandUnit(DX12CommandUnit** unit);
	uint64_t SubmitCommandUnit(ID3D12CommandList* unit);
	DX12RenderDevice* Device() const { return m_pDevice; }
public:
	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle(void) { WaitForFence(IncrementFence()); }
	bool IsFenceComplete(uint64_t fenceValue);
	uint64_t LastCompletedFence() const { return m_lastCompletedFenceValue; }
	void StallQueueForFence(uint64_t fenceValue);

	const COMMAND_UNIT_TYPE Type;
	uint64_t IncrementFence(void);
private:
	void updateLastCompletedFenceValue();
private:
	DX12RenderDevice* m_pDevice;
private:
	SmartPTR<ID3D12CommandQueue> m_commandQueue;
	CommandAllocatorPool m_allocatorPool;
	SmartPTR<ID3D12Fence> m_pFence;
	std::mutex m_fenceMutex;
	uint64_t m_nextFenceValue;
	uint64_t m_lastCompletedFenceValue;
	std::mutex m_eventMutex;
	HANDLE m_fenceEventHandle;
private:
	std::mutex m_freeCmdUnitMutex;
	std::mutex m_cmdUnitsMutex;
	std::queue<DX12CommandUnit*> m_freeCmdUnits;
	std::list<DX12CommandUnit> m_cmdUnits;
};

END_NAME_SPACE
