#include "DX12CommandUnit.h"
#include "../DX12RenderDevice.h"
#include <cassert>

BEG_NAME_SPACE

size_t DX12CommandUnit::Flush(bool bWaitForCompletion)
{
	auto& listMgr = m_pDevice->CommandListManager();
	auto& queue = listMgr.GetQueue(CommandUnitTypeToDX12CommandListType(CommandUnitType));
	size_t fenceValue = queue.ExecuteCommandList(m_graphicsCmdList.Get());
	/** 归还Allocator */
	queue.DiscardAllocator(fenceValue, m_pAllocator);
	Reset(queue.RequestAllocator()); /**< 重新申请一个Allocator */
	if (bWaitForCompletion) {
		queue.WaitForFence(fenceValue);
		cleanupFunction(fenceValue, queue)();
	}
	else {
		/** 还需要安排heap的释放以及临时资源的释放 */
		m_pDevice->AddAsyncTask(cleanupFunction(fenceValue, queue));
	}
	return fenceValue;
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

void DX12CommandUnit::BindVertexBuffers(size_t startSlot, size_t numberOfBuffers, GPUResource** ppBuffers)
{
	std::vector<D3D12_VERTEX_BUFFER_VIEW> vbvs(numberOfBuffers);
	for (size_t i = 0; i < numberOfBuffers; ++i) {
		vbvs[i] = dynamic_cast<DX12VertexBufferView*>(ppBuffers[i]->GetVBVPtr())->m_view;
	}
	m_graphicsCmdList->IASetVertexBuffers(startSlot, numberOfBuffers, vbvs.data());
}

void DX12CommandUnit::BindIndexBuffer(GPUResource * pBuffer)
{
	auto ibv =  dynamic_cast<DX12IndexBufferView*>(pBuffer->GetIBVPtr());
	m_graphicsCmdList->IASetIndexBuffer(&ibv->m_view);
}

void DX12CommandUnit::BindRenderTargets(GPUResource ** ppRenderTargets, size_t numRenderTargets, GPUResource* pDepthStencil)
{
	size_t RTVHead = m_transientDescriptorHeapForRTV.RequestBlock(numRenderTargets);
	auto dev = m_pDevice->GetDevice();
	for (size_t i = 0; i < numRenderTargets; ++i) {
		auto handle = m_transientDescriptorHeapForRTV.GetCPUHandle(RTVHead + i);
		auto pRTV = dynamic_cast<DX12RenderTargetView*>(ppRenderTargets[i]->GetRTVPtr());
		dev->CreateRenderTargetView(pRTV->m_res, &pRTV->m_desc, handle);
	}
	auto RTVHandle = m_transientDescriptorHeapForRTV.GetCPUHandle(RTVHead);
	if (pDepthStencil) {
		size_t DSVHead = m_transientDescriptorHeapForDSV.RequestBlock(1);
		auto DSVHandle = m_transientDescriptorHeapForDSV.GetCPUHandle(DSVHead);
		auto pDSV = dynamic_cast<DX12DepthStencilView*>(pDepthStencil->GetDSVPtr());
		dev->CreateDepthStencilView(pDSV->m_res, &pDSV->m_desc, DSVHandle);
		m_graphicsCmdList->OMSetRenderTargets(numRenderTargets, &RTVHandle, true, &DSVHandle);
		return;
	}
	/** TODO: 不清楚Depth Stencil是否非要不可 */
	m_graphicsCmdList->OMSetRenderTargets(numRenderTargets, &RTVHandle, true, nullptr);
}

void DX12CommandUnit::Draw(size_t startOfIndex, size_t indexCount, size_t startOfVertex)
{
	m_graphicsCmdList->DrawIndexedInstanced(indexCount, 1, startOfIndex, startOfVertex, 0);
}

void DX12CommandUnit::TransferState(GPUResource * pResource, ResourceStates newState)
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = reinterpret_cast<ID3D12Resource*>(pResource->GetResource());
	barrier.Transition.Subresource = 0;
	barrier.Transition.StateBefore = ResourceStateToDX12ResourceState(pResource->m_state);
	barrier.Transition.StateAfter = ResourceStateToDX12ResourceState(newState);
	pResource->m_state = newState;
	m_graphicsCmdList->ResourceBarrier(1, &barrier);
}

void DX12CommandUnit::Present()
{
	m_pDevice->Present();
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
	size_t RTVHead = m_transientDescriptorHeapForRTV.RequestBlock(1);
	auto dev = m_pDevice->GetDevice();
	auto pRTV = dynamic_cast<DX12RenderTargetView*>(renderTarget->GetRTVPtr());
	auto handle = m_transientDescriptorHeapForRTV.GetCPUHandle(RTVHead);
	dev->CreateRenderTargetView(pRTV->m_res, &pRTV->m_desc, handle);
	m_graphicsCmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
}

RenderDevice * DX12CommandUnit::GetDevice()
{
	return m_pDevice;
}

bool DX12CommandUnit::Initalize(
	DX12RenderDevice * pDevice,
	SmartPTR<ID3D12GraphicsCommandList> graphicsCmdList,
	ID3D12CommandAllocator * allocator)
{
	assert(pDevice != nullptr && graphicsCmdList != nullptr && allocator != nullptr);
	assert(m_pDevice == nullptr && m_graphicsCmdList == nullptr && m_pAllocator == nullptr);
	m_pDevice = pDevice;
	m_graphicsCmdList = graphicsCmdList;
	m_pAllocator = allocator;
	//m_graphicsCmdList->Reset(m_pAllocator, nullptr); 刚创建的commandList已经有一个匹配的Allocator而且当前CommandList已经处在open状态

	m_transientDescriptorHeapForGPU.Initialize(m_pDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_transientDescriptorHeapForRTV.Initialize(m_pDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false, 32);
	m_transientDescriptorHeapForDSV.Initialize(m_pDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false, 32);
	return true;
}

bool DX12CommandUnit::Reset(ID3D12CommandAllocator * allocator)
{
	assert(allocator != nullptr);
	assert(m_pDevice != nullptr && m_graphicsCmdList != nullptr);
	m_pAllocator = allocator;
	m_graphicsCmdList->Reset(m_pAllocator, nullptr);

	m_transientDescriptorHeapForGPU.Reset();
	m_transientDescriptorHeapForRTV.Reset();
	m_transientDescriptorHeapForDSV.Reset();
	return true;
}

void DX12CommandUnit::CopyBetweenBuffer(ID3D12Resource * destBuffer, ID3D12Resource * srcBuffer)
{
	m_graphicsCmdList->CopyResource(destBuffer, srcBuffer);
}

ID3D12Resource * DX12CommandUnit::TransientBuffer(size_t bufferSizeInByte, D3D12_HEAP_TYPE heapType)
{
	auto& resMgr = m_pDevice->ResourceManager();
	auto[pRes, state] = resMgr.RequestBuffer(bufferSizeInByte, D3D12_RESOURCE_FLAG_NONE, heapType, true);
	return pRes;
}

std::function<bool()> DX12CommandUnit::cleanupFunction(size_t fenceValue, DX12CommandQueue& queue)
{
	std::vector<ID3D12Resource*> transientResource;
	transientResource.swap(m_transientResources);
	TransientDX12DescriptorHeap transientDescriptorHeapForDSV = m_transientDescriptorHeapForDSV;
	TransientDX12DescriptorHeap transientDescriptorHeapForRTV = m_transientDescriptorHeapForRTV;
	TransientDX12DescriptorHeap transientDescriptorHeapForGPU = m_transientDescriptorHeapForGPU;
	m_transientDescriptorHeapForGPU.Initialize(m_pDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_transientDescriptorHeapForRTV.Initialize(m_pDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false, 32);
	m_transientDescriptorHeapForDSV.Initialize(m_pDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false, 32);

	DX12RenderDevice* pDevice = m_pDevice;
	return
		[transientResource, fenceValue, &queue, pDevice, 
		transientDescriptorHeapForRTV, 
		transientDescriptorHeapForDSV,
		transientDescriptorHeapForGPU]()mutable->bool {
		if (queue.IsFenceComplete(fenceValue)) {
			/** 指令执行完成，释放资源 */
			auto& resMgr = pDevice->ResourceManager();
			for (auto resource : transientResource) {
				assert(resMgr.ReleaseResource(resource));
			}
			transientDescriptorHeapForRTV.Reset();
			transientDescriptorHeapForDSV.Reset();
			transientDescriptorHeapForGPU.Reset();
			return true;
		}
		else {
			return false;
		}
	};
}

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


bool DX12CommandUnitManager::Create(DX12RenderDevice * pDevice)
{
	assert(pDevice != nullptr);
	m_pDevice = pDevice;
	m_pCmdListManager = &pDevice->CommandListManager();
	return true;
}

DX12CommandUnit* DX12CommandUnitManager::AllocateCommandUnit(COMMAND_UNIT_TYPE type) {
	DX12CommandUnit* pAllocate = nullptr;
	{
		std::lock_guard<std::mutex> lg(m_freeCmdUnitMutex);
		if (m_freeCmdUnits[type].empty() == false) {
			pAllocate = m_freeCmdUnits[type].front();
			m_freeCmdUnits[type].pop();
			/** 已经创建过的CommandUnit应该初始化过，拥有固定的device和CommandList指向 */
			assert(pAllocate->m_pDevice != nullptr && pAllocate->m_graphicsCmdList != nullptr && pAllocate->m_pAllocator == nullptr);
			assert(pAllocate->Reset(m_pCmdListManager->GetQueue(CommandUnitTypeToDX12CommandListType(type)).RequestAllocator()));
		}
	}
	/** 没有空闲的CommandUnit，需要重新创建 */
	if (pAllocate == nullptr) {
		DX12CommandUnit newUnit(type);
		SmartPTR<ID3D12GraphicsCommandList> graphicsCmdList;
		ID3D12CommandAllocator* pCommandAllocator;
		m_pCmdListManager->CreateNewCommandList(
			CommandUnitTypeToDX12CommandListType(type),
			graphicsCmdList,
			&pCommandAllocator
		);
		newUnit.Initalize(m_pDevice, graphicsCmdList, pCommandAllocator);
		std::lock_guard<std::mutex> lg(m_cmdUnitsMutex);
		m_cmdUnits->push_back(newUnit);
		pAllocate = &m_cmdUnits->back();
	}
	assert(pAllocate != nullptr);
	return pAllocate;
}

void DX12CommandUnitManager::FreeCommandUnit(DX12CommandUnit ** unit)
{
	std::lock_guard<std::mutex> lg(m_freeCmdUnitMutex);
	(*unit)->m_graphicsCmdList->ClearState(nullptr);
	if ((*unit)->m_pAllocator != nullptr) {
		m_pCmdListManager->GetQueue(
			CommandUnitTypeToDX12CommandListType((*unit)->CommandUnitType))
			.DiscardAllocator(0, (*unit)->m_pAllocator);
		(*unit)->m_pAllocator = nullptr;
	}
	/** 释放所有的临时资源 */
	(*unit)->cleanupFunction(0, m_pCmdListManager->GetQueue(
		CommandUnitTypeToDX12CommandListType((*unit)->CommandUnitType)))();
	
	m_freeCmdUnits->push(*unit);
	*unit = nullptr;
}

END_NAME_SPACE
