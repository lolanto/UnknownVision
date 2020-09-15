#include "DX12CommandUnit.h"
#include "DX12RenderDevice.h"
#include "DX12GPUResource.h"
#include "DX12Pipeline.h"
#include "DX12BindingBoard.h"
#include <iostream>

BEG_NAME_SPACE

size_t DX12CommandUnit::Flush(bool bWaitForCompletion)
{
	m_graphicsCmdList->Close();
	m_lastFenceValue = m_pMgr->SubmitCommandUnit(m_graphicsCmdList.Get());
	if (FAILED(m_graphicsCmdList->Reset(m_pAllocator, nullptr))) {
		LOG_ERROR("Reset graphics command list failed!");
		abort();
	}
	loopValidBindingBoard([this](std::pair<size_t, DX12BindingBoard*> board) {
		board.second->SetLastFenceValue(m_lastFenceValue);
		});
	if (bWaitForCompletion) {
		m_pMgr->WaitForFence(m_lastFenceValue);
	}
	m_bindingSlotToBindingBoards.clear();
	return m_lastFenceValue;
}

void DX12CommandUnit::BindPipeline(GraphicsPipelineObject * gpo)
{
	DX12GraphicsPipelineObject* dxGPO = dynamic_cast<DX12GraphicsPipelineObject*>(gpo);
	m_graphicsCmdList->SetPipelineState(dxGPO->GetPSO());
	m_graphicsCmdList->SetGraphicsRootSignature(dxGPO->GetRootSignature());
	m_graphicsCmdList->IASetPrimitiveTopology(
		PrimitiveTypeToPrimitiveTopology(dxGPO->rastOpt().primitive)
	);
}

void DX12CommandUnit::SetBindingBoard(size_t slot, BindingBoard* board)
{
	m_bindingSlotToBindingBoards[slot] = board;
}

void DX12CommandUnit::BindVertexBuffers(size_t startSlot, size_t numberOfBuffers, Buffer** ppBuffers)
{
	/** Note: 假如CommandUnit被多个线程使用，这个vbv就可能产生冲突 */
	static D3D12_VERTEX_BUFFER_VIEW vbvs[MAX_VERTEX_BUFFER];
	if (numberOfBuffers > MAX_VERTEX_BUFFER) {
		LOG_ERROR("You can't bind buffers more than %d, number of buffer will be binded is %d", MAX_VERTEX_BUFFER, numberOfBuffers);
		abort();
	}
	for (size_t i = 0; i < numberOfBuffers; ++i) {
		vbvs[i] = dynamic_cast<DX12Buffer*>(ppBuffers[i])->GetVertexBufferView();
	}
	m_graphicsCmdList->IASetVertexBuffers(startSlot, numberOfBuffers, vbvs);
}

void DX12CommandUnit::BindIndexBuffer(Buffer * pBuffer)
{
	/** Note: 假如 */
	static D3D12_INDEX_BUFFER_VIEW idxvbv;
	auto pBuf =  dynamic_cast<DX12Buffer*>(pBuffer);
	idxvbv = pBuf->GetIndexBufferView();
	m_graphicsCmdList->IASetIndexBuffer(&idxvbv);
}

void DX12CommandUnit::BindRenderTargets(GPUResource** ppRenderTargets, size_t numRenderTargets, GPUResource* pDepthStencil)
{
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandles[MAX_RENDER_TARGET];
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle;
	for (size_t i = 0; i < numRenderTargets; ++i) {
		RTVHandles[i] = m_pDevice->PickupRenderTarget(ppRenderTargets[i]).value();
	}
	/** TODO: 还需要设置DSV */
	if (pDepthStencil != nullptr) {
		DSVHandle = m_pDevice->PickupDepthStencilTarget(pDepthStencil).value();
		m_graphicsCmdList->OMSetRenderTargets(numRenderTargets, RTVHandles, true, &DSVHandle);
	}
	else {
		m_graphicsCmdList->OMSetRenderTargets(numRenderTargets, RTVHandles, true, nullptr);
	}
}

void DX12CommandUnit::Draw(size_t startOfIndex, size_t indexCount, size_t startOfVertex)
{
	ID3D12DescriptorHeap* heap = m_pDevice->GetDescriptorHeap(CommandUnitType);
	m_graphicsCmdList->SetDescriptorHeaps(1, &heap);
	loopValidBindingBoard([this](std::pair<size_t, DX12BindingBoard*> board) {
		m_graphicsCmdList->SetGraphicsRootDescriptorTable(board.first, board.second->AllocateDescriptorHeap().gpuHandle);
		});
	m_graphicsCmdList->DrawIndexedInstanced(indexCount, 1, startOfIndex, startOfVertex, 0);
}

void DX12CommandUnit::TransferState(GPUResource * pResource, ResourceStates newState)
{
	if (pResource->m_status.isStably() == false) return; /**< 这样设置是为了应对在主存上的constant buffer，它们的state一直是generic read，但依旧可以绑定到cb上，且不需要进行任何状态变化 */
	if (newState == pResource->m_state) return; /**< 状态一致不进行变更 */
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = reinterpret_cast<ID3D12Resource*>(pResource->GetResource());
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = ResourceStateToDX12ResourceState(pResource->m_state);
	barrier.Transition.StateAfter = ResourceStateToDX12ResourceState(newState);
	pResource->m_state = newState;
	m_graphicsCmdList->ResourceBarrier(1, &barrier);
}

void DX12CommandUnit::BindViewports(size_t size, ViewPort * viewports)
{
	std::vector<D3D12_VIEWPORT> vps(size);
	for (size_t i = 0; i < size; ++i) {
		vps[i] = ViewPortToDX12ViewPort(viewports[i]);
	}
	m_graphicsCmdList->RSSetViewports(size, vps.data());
}

void DX12CommandUnit::BindScissorRects(size_t size, ScissorRect * scissorRects)
{
	std::vector<D3D12_RECT> srs(size);
	for (size_t i = 0; i < size; ++i) {
		srs[i] = ScissorRectToDX12ScissorRect(scissorRects[i]);
	}
	m_graphicsCmdList->RSSetScissorRects(size, srs.data());
}

void DX12CommandUnit::ClearRenderTarget(GPUResource * renderTarget, const float * clearColor)
{
	auto handle = m_pDevice->PickupRenderTarget(renderTarget);
	m_graphicsCmdList->ClearRenderTargetView(handle.value(), clearColor, 0, nullptr);
}

void DX12CommandUnit::ClearDepthStencilBuffer(GPUResource* ds, float depth, uint8_t stencil)
{
	auto handle = m_pDevice->PickupDepthStencilTarget(ds);
	m_graphicsCmdList->ClearDepthStencilView(handle.value(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil ,0, nullptr);
}

RenderDevice * DX12CommandUnit::GetDevice()
{
	return m_pMgr->Device();
}

bool DX12CommandUnit::Initalize(
	DX12CommandUnitManager * pMgr,
	SmartPTR<ID3D12GraphicsCommandList> graphicsCmdList,
	ID3D12CommandAllocator * allocator,
	DX12RenderDevice* pDevice)
{
	if (pMgr == nullptr || graphicsCmdList == nullptr || allocator == nullptr) {
		LOG_WARN("Invalid parameter(s)");
		return false;
	}
	if (m_pMgr != nullptr || m_graphicsCmdList != nullptr || m_pAllocator != nullptr) {
		LOG_ERROR("Don't Initialize command unit twice!");
		abort();
	}
	m_pMgr = pMgr;
	m_graphicsCmdList = graphicsCmdList;
	m_pAllocator = allocator;
	m_pDevice = pDevice;
	return true;
}

bool DX12CommandUnit::Reset(ID3D12CommandAllocator * allocator)
{
	if (allocator == nullptr) {
		LOG_WARN("Reset Allocator can not be null");
		return false;
	}
	if (m_pMgr == nullptr || m_graphicsCmdList == nullptr) {
		LOG_ERROR("Can not call reset until you initialized this unit!");
		abort();
	}
	m_pAllocator = allocator;
	m_graphicsCmdList->Reset(m_pAllocator, nullptr);

	return true;
}

void DX12CommandUnit::CopyBetweenBuffer(ID3D12Resource * destBuffer, ID3D12Resource * srcBuffer)
{
	m_graphicsCmdList->CopyResource(destBuffer, srcBuffer);
}

void DX12CommandUnit::loopValidBindingBoard(std::function<void(std::pair<size_t, DX12BindingBoard*>)> func)
{
	for (const auto& board : m_bindingSlotToBindingBoards) {
		if (board.second != nullptr) {
			func({ board.first, dynamic_cast<DX12BindingBoard*>(board.second) });
		}
	}
}

//ID3D12Resource * DX12CommandUnit::TransientBuffer(size_t bufferSizeInByte, D3D12_HEAP_TYPE heapType)
//{
//	auto& resMgr = m_pDevice->ResourceManager();
//	auto[pRes, state] = resMgr.RequestBuffer(bufferSizeInByte, D3D12_RESOURCE_FLAG_NONE, heapType, true);
//	return pRes;
//}

//std::function<bool()> DX12CommandUnit::cleanupFunction(size_t fenceValue, DX12CommandQueue& queue)
//{
//	std::vector<ID3D12Resource*> transientResource;
//	transientResource.swap(m_transientResources);
//	TransientDX12DescriptorHeap transientDescriptorHeapForGPU = m_transientDescriptorHeapForGPU;
//	m_transientDescriptorHeapForGPU.Initialize(m_pDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
//
//	DX12RenderDevice* pDevice = m_pDevice;
//	return
//		[transientResource, fenceValue, &queue, pDevice, 
//		transientDescriptorHeapForGPU]()mutable->bool {
//		if (queue.IsFenceComplete(fenceValue)) {
//			/** 指令执行完成，释放资源 */
//			auto& resMgr = pDevice->ResourceManager();
//			for (auto resource : transientResource) {
//				assert(resMgr.ReleaseResource(resource));
//			}
//			transientDescriptorHeapForGPU.Reset();
//			return true;
//		}
//		else {
//			return false;
//		}
//	};
//}

//void DX12CommandUnit::Setup(SmartPTR<ID3D12CommandQueue> queue) {
//	assert(m_device != nullptr);
//	initializeFence();
//	initializeCommandAllocAndList();
//}
//
//void DX12CommandUnit::Setup(D3D12_COMMAND_LIST_TYPE) {
//	D3D12_COMMAND_QUEUE_DESC queueDesc;
//	assert(m_device != nullptr);
//	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
//	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//	queueDesc.NodeMask = 0;
//	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
//	assert(SUCCEEDED(m_device->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue))));
//	initializeFence();
//	initializeCommandAllocAndList();
//}

//inline void DX12CommandUnit::initializeFence() {
//	/** 初始值是0，则下一个fenceValue必须是1 */
//	assert(SUCCEEDED(m_device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))));
//	m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	m_nextFenceValue = 1;
//}

//inline void DX12CommandUnit::initializeCommandAllocAndList() {
//	m_device->GetDevice()->CreateCommandAllocator(m_queue->GetDesc().Type, IID_PPV_ARGS(&m_alloc));
//	m_device->GetDevice()->CreateCommandList(0, m_queue->GetDesc().Type, m_alloc.Get(), nullptr, IID_PPV_ARGS(&m_graphicCmdList));
//	m_graphicCmdList->Close(); /**< 创建完成先close */
//}

//bool DX12CommandUnit::Active() {
//	m_graphicCmdList->Reset(m_alloc.Get(), nullptr);
//	return true;
//}
//
//bool DX12CommandUnit::Fetch() {
//	m_graphicCmdList->Close();
//	if (m_executing) Wait();
//	ID3D12CommandList* list = m_graphicCmdList.Get();
//	m_queue->ExecuteCommandLists(1, &list);
//	m_lastFenceValue = m_nextFenceValue++;
//	m_queue->Signal(m_fence.Get(), m_lastFenceValue);
//	m_executing = true;
//	return true;
//}
//
//bool DX12CommandUnit::FetchAndPresent()
//{
//	m_graphicCmdList->Close();
//	if (m_executing) Wait();
//	ID3D12CommandList* list = m_graphicCmdList.Get();
//	m_queue->ExecuteCommandLists(1, &list);
//	m_lastFenceValue = m_nextFenceValue++;
//	if (m_device->Present() == false) return false;
//	m_queue->Signal(m_fence.Get(), m_lastFenceValue);
//	m_executing = true;
//}
//
//bool DX12CommandUnit::Wait() {
//	if (m_executing) {
//		while (m_fence->GetCompletedValue() < m_lastFenceValue) {
//			m_fence->SetEventOnCompletion(m_lastFenceValue, m_fenceEvent);
//			WaitForSingleObject(m_fenceEvent, INFINITE);
//		}
//		m_executing = false;
//		for (auto& e : m_onEndEvent) e();
//		m_onEndEvent.clear();
//		for (auto& e : m_onEndEvent_regular) e();
//	}
//	return true;
//}
//
//bool DX12CommandUnit::RegisterEvent(std::function<void()> e, EventTime time, bool isRegularEvent)
//{
//	switch (time) {
//	case EVENT_TIME_ON_END:
//		if (isRegularEvent) {
//			m_onEndEvent_regular.push_back(e);
//		}
//		else {
//			m_onEndEvent.push_back(e);
//		}
//		break;
//	}
//	return true;
//}
//
//bool DX12CommandUnit::Reset()
//{
//	Wait();
//	m_alloc->Reset();
//	m_onEndEvent.clear();
//	m_onEndEvent_regular.clear();
//	return false;
//}
//
//bool DX12CommandUnit::UpdateBufferWithSysMem(BufferHandle dest, void * src, size_t size)
//{
//	BufferInfo* bufInfoPtr = m_device->PickupBuffer(dest);
//	if (bufInfoPtr == nullptr) {
//		FLOG("%s: Invalid buffer handle!\n", __FUNCTION__);
//		return false;
//	}
//	/** 确认能否更新 */
//	if (bufInfoPtr->size < size) {
//		FLOG("%s: Size of target buffer is less than %zu, update failed!", __FUNCTION__, size);
//		return false;
//	}
//	D3D12_HEAP_PROPERTIES prop;
//	D3D12_HEAP_FLAGS flag;
//	bufInfoPtr->ptr->GetHeapProperties(&prop, &flag);
//	if (prop.Type != D3D12_HEAP_TYPE_UPLOAD) {
//		FLOG("%s: CPU can't access this buffer!\n", __FUNCTION__);
//		return false;
//	}
//
//	/** 直接map并修改 */
//	void* memPtr = nullptr;
//	bufInfoPtr->ptr->Map(0, nullptr, &memPtr);
//	memcpy(memPtr, src, size);
//	bufInfoPtr->ptr->Unmap(0, nullptr);
//
//	return true;
//}
//
//bool DX12CommandUnit::ReadBackToSysMem(BufferHandle src, void * dest, size_t size)
//{
//	BufferInfo* bufInfoPtr = m_device->PickupBuffer(src);
//	if (bufInfoPtr == nullptr) {
//		FLOG("%s: Invalid buffer handle!\n", __FUNCTION__);
//		return false;
//	}
//	D3D12_HEAP_PROPERTIES prop;
//	D3D12_HEAP_FLAGS flag;
//	bufInfoPtr->ptr->GetHeapProperties(&prop, &flag);
//	if (bufInfoPtr->size > size) {
//		FLOG("%s: Size of target buffer is less than %zu, update failed!", __FUNCTION__, size);
//		return false;
//	}
//	if (prop.Type != D3D12_HEAP_TYPE_READBACK &&
//		prop.Type != D3D12_HEAP_TYPE_UPLOAD) {
//		FLOG("%s: CPU can't access this buffer\n", __FUNCTION__);
//		return false;
//	}
//
//	void* memPtr = nullptr;
//	bufInfoPtr->ptr->Map(0, nullptr, &memPtr);
//	memcpy(dest, memPtr, size);
//	bufInfoPtr->ptr->Unmap(0, nullptr);
//
//	return true;
//}
//
//bool DX12CommandUnit::CopyBetweenGPUBuffer(BufferHandle src, BufferHandle dest, size_t srcOffset, size_t destOffset, size_t size)
//{
//	BufferInfo* srcBufInfoPtr = m_device->PickupBuffer(src);
//	if (srcBufInfoPtr == nullptr) {
//		FLOG("%s: Invalid source buffer handle\n", __FUNCTION__);
//		return false;
//	}
//	BufferInfo* destBufInfoPtr = m_device->PickupBuffer(dest);
//	if (destBufInfoPtr == nullptr) {
//		FLOG("%s: Invalid dest buffer handle\n", __FUNCTION__);
//		return false;
//	}
//	
//	m_graphicCmdList->CopyBufferRegion(destBufInfoPtr->ptr, destOffset, srcBufInfoPtr->ptr, srcOffset, size);
//	return true;
//}
//
//bool DX12CommandUnit::TransferState(BufferHandle buf, ResourceStates newState)
//{
//	BufferInfo* bufInfoPtr = m_device->PickupBuffer(buf);
//	if (bufInfoPtr == nullptr) {
//		FLOG("%s: Invalid buffer handle\n", __FUNCTION__);
//		return false;
//	}
//	D3D12_HEAP_PROPERTIES prop;
//	D3D12_HEAP_FLAGS flag;
//	bufInfoPtr->ptr->GetHeapProperties(&prop, &flag);
//	if (prop.Type == D3D12_HEAP_TYPE_UPLOAD || prop.Type == D3D12_HEAP_TYPE_READBACK) {
//		MLOG("resource on upload/readback heap can't change state!\n");
//		return true;
//	}
//	if (stateMatch(bufInfoPtr->state, newState)) return true;
//	D3D12_RESOURCE_BARRIER barrier;
//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.pResource = bufInfoPtr->ptr;
//	barrier.Transition.StateBefore = bufInfoPtr->state;
//	barrier.Transition.StateAfter = ResourceStateToDX12ResourceState(newState);
//	barrier.Transition.Subresource = 0;
//	m_graphicCmdList->ResourceBarrier(1, &barrier);
//	return true;
//}
//
//bool DX12CommandUnit::TransferState(Buffer* buf, ResourceStates newState) {
//	DX12Buffer* dx12Buf = dynamic_cast<DX12Buffer*>(buf);
//	assert(dx12Buf != nullptr);
//	D3D12_HEAP_PROPERTIES prop;
//	D3D12_HEAP_FLAGS flag;
//	dx12Buf->GetResource()->GetHeapProperties(&prop, &flag);
//	if (prop.Type == D3D12_HEAP_TYPE_UPLOAD || prop.Type == D3D12_HEAP_TYPE_READBACK) {
//		MLOG("resource on upload/readback heap can't change state!\n");
//		return true;
//	}
//	if (stateMatch(dx12Buf->GetResourceState(), newState)) return true;
//	D3D12_RESOURCE_BARRIER barrier;
//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.pResource = dx12Buf->GetResource();
//	barrier.Transition.StateBefore = dx12Buf->GetResourceState();
//	barrier.Transition.StateAfter = ResourceStateToDX12ResourceState(newState);
//	barrier.Transition.Subresource = 0;
//	m_graphicCmdList->ResourceBarrier(1, &barrier);
//	/** 修改对应的资源状态 */
//	dx12Buf->GetResourceState() = ResourceStateToDX12ResourceState(newState);
//	return true;
//}
//
//bool DX12CommandUnit::TransferState(TextureHandle tex, ResourceStates newState)
//{
//	TextureInfo* texInfoPtr = m_device->PickupTexture(tex);
//	if (texInfoPtr == nullptr) {
//		FLOG("%s: Invalid texture handle\n", __FUNCTION__);
//		return false;
//	}
//	D3D12_HEAP_PROPERTIES prop;
//	D3D12_HEAP_FLAGS flag;
//	texInfoPtr->ptr->GetHeapProperties(&prop, &flag);
//	if (prop.Type == D3D12_HEAP_TYPE_UPLOAD || prop.Type == D3D12_HEAP_TYPE_READBACK) {
//		MLOG("resource on upload/readback heap can't change state!\n");
//		return true;
//	}
//	if (stateMatch(texInfoPtr->state, newState)) return true;
//	D3D12_RESOURCE_BARRIER barrier;
//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.pResource = texInfoPtr->ptr;
//	barrier.Transition.StateBefore = texInfoPtr->state;
//	barrier.Transition.StateAfter = texInfoPtr->state = ResourceStateToDX12ResourceState(newState);
//	/** TODO: 暂时只支持对子元素0的状态变更 */
//	barrier.Transition.Subresource = 0;
//	m_graphicCmdList->ResourceBarrier(1, &barrier);
//	return true;
//}
//
//bool DX12CommandUnit::BindRenderTargetsAndDepthStencilBuffer(const std::vector<TextureHandle>& renderTargets, TextureHandle depthStencil)
//{
//	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles(renderTargets.size());
//	for (size_t index = 0; index < rtvHandles.size(); ++index) {
//		auto value = m_device->PickupRenderTarget(renderTargets[index]);
//		if (!value) {
//			FLOG("%s: Pick up render target failed!\n", __FUNCTION__);
//			return false;
//		}
//		rtvHandles[index] = value.value();
//	}
//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
//	{
//		auto value = m_device->PickupDepthStencilTarget(depthStencil);
//		if (!value) {
//			FLOG("%s: Pick up depth stencil failed!\n", __FUNCTION__);
//			return false;
//		}
//		dsvHandle = value.value();
//	}
//	m_graphicCmdList->OMSetRenderTargets(rtvHandles.size(), rtvHandles.data(), false, &dsvHandle);
//	return true;
//}
//
//bool DX12CommandUnit::ClearRenderTarget(TextureHandle renderTarget, const std::array<float, 4>& color)
//{
//	auto value = m_device->PickupRenderTarget(renderTarget);
//	if (!value) {
//		FLOG("%s: Pick up render target failed!\n", __FUNCTION__);
//		return false;
//	}
//	m_graphicCmdList->ClearRenderTargetView(value.value(), color.data(), 0, NULL);
//	return true;
//}


//bool DX12CommandUnitManager::Create(DX12RenderDevice * pDevice)
//{
//	assert(pDevice != nullptr);
//	m_pDevice = pDevice;
//	m_pCmdListManager = &pDevice->CommandListManager();
//	return true;
//}
//
//DX12CommandUnit* DX12CommandUnitManager::AllocateCommandUnit(COMMAND_UNIT_TYPE type) {
//	DX12CommandUnit* pAllocate = nullptr;
//	{
//		std::lock_guard<std::mutex> lg(m_freeCmdUnitMutex);
//		if (m_freeCmdUnits[type].empty() == false) {
//			pAllocate = m_freeCmdUnits[type].front();
//			m_freeCmdUnits[type].pop();
//			/** 已经创建过的CommandUnit应该初始化过，拥有固定的device和CommandList指向 */
//			assert(pAllocate->m_pDevice != nullptr && pAllocate->m_graphicsCmdList != nullptr && pAllocate->m_pAllocator == nullptr);
//			assert(pAllocate->Reset(m_pCmdListManager->GetQueue(CommandUnitTypeToDX12CommandListType(type)).RequestAllocator()));
//		}
//	}
//	/** 没有空闲的CommandUnit，需要重新创建 */
//	if (pAllocate == nullptr) {
//		DX12CommandUnit newUnit(type);
//		SmartPTR<ID3D12GraphicsCommandList> graphicsCmdList;
//		ID3D12CommandAllocator* pCommandAllocator;
//		m_pCmdListManager->CreateNewCommandList(
//			CommandUnitTypeToDX12CommandListType(type),
//			graphicsCmdList,
//			&pCommandAllocator
//		);
//		newUnit.Initalize(m_pDevice, graphicsCmdList, pCommandAllocator);
//		std::lock_guard<std::mutex> lg(m_cmdUnitsMutex);
//		m_cmdUnits->push_back(newUnit);
//		pAllocate = &m_cmdUnits->back();
//	}
//	assert(pAllocate != nullptr);
//	return pAllocate;
//}
//
//void DX12CommandUnitManager::FreeCommandUnit(DX12CommandUnit ** unit)
//{
//	std::lock_guard<std::mutex> lg(m_freeCmdUnitMutex);
//	(*unit)->m_graphicsCmdList->ClearState(nullptr);
//	if ((*unit)->m_pAllocator != nullptr) {
//		m_pCmdListManager->GetQueue(
//			CommandUnitTypeToDX12CommandListType((*unit)->CommandUnitType))
//			.DiscardAllocator(0, (*unit)->m_pAllocator);
//		(*unit)->m_pAllocator = nullptr;
//	}
//	/** 释放所有的临时资源 */
//	//(*unit)->cleanupFunction(0, m_pCmdListManager->GetQueue(
//	//	CommandUnitTypeToDX12CommandListType((*unit)->CommandUnitType)))();
//	
//	m_freeCmdUnits->push(*unit);
//	*unit = nullptr;
//}

DX12CommandUnitManager::~DX12CommandUnitManager() {
	if (m_commandQueue == nullptr)
		return;
	WaitForIdle();
	m_allocatorPool.Shutdown();
	m_commandQueue.Reset();
	m_pFence.Reset();
	CloseHandle(m_fenceEventHandle);
}

bool DX12CommandUnitManager::Create(DX12RenderDevice* pDevice,
	SmartPTR<ID3D12CommandQueue> queue) {
	m_pDevice = pDevice;
	if (queue != nullptr) {
		assert(queue->GetDesc().Type == CommandUnitTypeToDX12CommandListType(Type));
		m_commandQueue = queue;
	}
	else {
		D3D12_COMMAND_QUEUE_DESC desc;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type = CommandUnitTypeToDX12CommandListType(Type);
		desc.NodeMask = m_pDevice->NodeMask;
		assert(SUCCEEDED(m_pDevice->GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue))));
	}
	assert(SUCCEEDED(m_pDevice->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))));
	m_allocatorPool.Create(m_pDevice->GetDevice());
	m_fenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
	assert(m_fenceEventHandle != INVALID_HANDLE_VALUE);
	m_nextFenceValue = 1;
	m_lastCompletedFenceValue = 0;

	return true;
}

DX12CommandUnit* DX12CommandUnitManager::AllocateCommandUnit() {
		DX12CommandUnit* pAllocate = nullptr;
	{
		std::lock_guard<std::mutex> lg(m_freeCmdUnitMutex);
		if (m_freeCmdUnits.empty() == false) {
			pAllocate = m_freeCmdUnits.front();
			m_freeCmdUnits.pop();
			/** 已经创建过的CommandUnit应该初始化过，拥有固定的device和CommandList指向 */
			assert(pAllocate->m_pMgr != nullptr && pAllocate->m_graphicsCmdList != nullptr && pAllocate->m_pAllocator == nullptr);
			assert(pAllocate->Reset(m_allocatorPool.RequestAllocator(m_lastCompletedFenceValue)));
		}
	}
	/** 没有空闲的CommandUnit，需要重新创建 */
	if (pAllocate == nullptr) {
		DX12CommandUnit newUnit(Type);
		SmartPTR<ID3D12GraphicsCommandList> graphicsCmdList;
		ID3D12CommandAllocator* pCommandAllocator = m_allocatorPool.RequestAllocator(m_lastCompletedFenceValue);
		m_pDevice->GetDevice()->CreateCommandList(
			m_pDevice->NodeMask, CommandUnitTypeToDX12CommandListType(Type),
			pCommandAllocator, nullptr, IID_PPV_ARGS(&graphicsCmdList));
		newUnit.Initalize(this, graphicsCmdList, pCommandAllocator, m_pDevice);
		std::lock_guard<std::mutex> lg(m_cmdUnitsMutex);
		m_cmdUnits.push_back(newUnit);
		pAllocate = &m_cmdUnits.back();
	}
	assert(pAllocate != nullptr);
	return pAllocate;
}

void DX12CommandUnitManager::FreeCommandUnit(DX12CommandUnit** unit) {
	std::lock_guard<std::mutex> lg(m_freeCmdUnitMutex);
	(*unit)->m_graphicsCmdList->ClearState(nullptr);
	(*unit)->m_graphicsCmdList->Close();
	if ((*unit)->m_pAllocator != nullptr) {
		m_allocatorPool.DiscardAllocator((*unit)->m_lastFenceValue, (*unit)->m_pAllocator);
		(*unit)->m_pAllocator = nullptr;
	}

	m_freeCmdUnits.push(*unit);
	*unit = nullptr;
}

uint64_t DX12CommandUnitManager::SubmitCommandUnit(ID3D12CommandList* cmdList) {
	std::lock_guard<std::mutex> lg(m_fenceMutex);
	// note: 进入之前保证ID3D12CommandList已经调用close了
	ID3D12CommandList* lists[] = { cmdList };
	m_commandQueue->ExecuteCommandLists(1, lists);
	m_commandQueue->Signal(m_pFence.Get(), m_nextFenceValue);
	return m_nextFenceValue++;
}

void DX12CommandUnitManager::WaitForFence(uint64_t fenceValue) {
	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
   // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
   // the fence can only have one event set on completion, then thread B has to wait for 
   // 100 before it knows 99 is ready.  Maybe insert sequential events?
	std::lock_guard<std::mutex> LockGuard(m_eventMutex);
	if (IsFenceComplete(fenceValue))
		return;

	m_pFence->SetEventOnCompletion(fenceValue, m_fenceEventHandle);
	WaitForSingleObject(m_fenceEventHandle, INFINITE);
	m_lastCompletedFenceValue = std::max(fenceValue, m_lastCompletedFenceValue);
}

bool DX12CommandUnitManager::IsFenceComplete(uint64_t fenceValue) {
	// Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	/** 这里没有加锁，lastCompletedFenceValue的值确实会因为竞争而出错，
	 * 但由于max每次都将GetCompletedValue纳入考虑范围，所以比较结果应该在大部分情况下
	 * 是正确的，但没办法保证写入的lastCompletedFenceValue是否正确 */
	if (fenceValue > m_lastCompletedFenceValue) {
		m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_pFence->GetCompletedValue());
	}

	return fenceValue <= m_lastCompletedFenceValue;
}

void DX12CommandUnitManager::updateLastCompletedFenceValue()
{
	m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_pFence->GetCompletedValue());
}

void DX12CommandUnitManager::StallQueueForFence(uint64_t fenceValue) {
	m_commandQueue->Wait(m_pFence.Get(), fenceValue);
}

uint64_t DX12CommandUnitManager::IncrementFence(void) {
	std::lock_guard<std::mutex> LockGuard(m_fenceMutex);
	m_commandQueue->Signal(m_pFence.Get(), m_nextFenceValue);
	return m_nextFenceValue++;
}

END_NAME_SPACE
