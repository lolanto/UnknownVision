#include "DX12CommandUnit.h"
#include "DX12RenderBasic.h"
#include "DX12RenderResource.h"
#include <cassert>

BEG_NAME_SPACE

void DX12CommandUnit::Setup(SmartPTR<ID3D12CommandQueue> queue) {
	assert(m_device != nullptr);
	m_queue = queue;
	initializeFence();
	initializeCommandAllocAndList();
}

void DX12CommandUnit::Setup(D3D12_COMMAND_LIST_TYPE) {
	D3D12_COMMAND_QUEUE_DESC queueDesc;
	assert(m_device != nullptr);
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	assert(SUCCEEDED(m_device->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue))));
	initializeFence();
	initializeCommandAllocAndList();
}

inline void DX12CommandUnit::initializeFence() {
	/** 初始值是0，则下一个fenceValue必须是1 */
	assert(SUCCEEDED(m_device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))));
	m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_nextFenceValue = 1;
}

inline void DX12CommandUnit::initializeCommandAllocAndList() {
	m_device->GetDevice()->CreateCommandAllocator(m_queue->GetDesc().Type, IID_PPV_ARGS(&m_alloc));
	m_device->GetDevice()->CreateCommandList(0, m_queue->GetDesc().Type, m_alloc.Get(), nullptr, IID_PPV_ARGS(&m_graphicCmdList));
	m_graphicCmdList->Close(); /**< 创建完成先close */
}

bool DX12CommandUnit::Active() {
	m_graphicCmdList->Reset(m_alloc.Get(), nullptr);
	return true;
}

bool DX12CommandUnit::Fetch() {
	m_graphicCmdList->Close();
	if (m_executing) Wait();
	ID3D12CommandList* list = m_graphicCmdList.Get();
	m_queue->ExecuteCommandLists(1, &list);
	m_lastFenceValue = m_nextFenceValue++;
	m_queue->Signal(m_fence.Get(), m_lastFenceValue);
	m_executing = true;
	return true;
}

bool DX12CommandUnit::FetchAndPresent()
{
	m_graphicCmdList->Close();
	if (m_executing) Wait();
	ID3D12CommandList* list = m_graphicCmdList.Get();
	m_queue->ExecuteCommandLists(1, &list);
	m_lastFenceValue = m_nextFenceValue++;
	if (m_device->Present() == false) return false;
	m_queue->Signal(m_fence.Get(), m_lastFenceValue);
	m_executing = true;
}

bool DX12CommandUnit::Wait() {
	if (m_executing) {
		while (m_fence->GetCompletedValue() < m_lastFenceValue) {
			m_fence->SetEventOnCompletion(m_lastFenceValue, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
		m_executing = false;
		for (auto& e : m_onEndEvent) e();
		m_onEndEvent.clear();
		for (auto& e : m_onEndEvent_regular) e();
	}
	return true;
}

bool DX12CommandUnit::RegisterEvent(std::function<void()> e, EventTime time, bool isRegularEvent)
{
	switch (time) {
	case EVENT_TIME_ON_END:
		if (isRegularEvent) {
			m_onEndEvent_regular.push_back(e);
		}
		else {
			m_onEndEvent.push_back(e);
		}
		break;
	}
	return true;
}

bool DX12CommandUnit::Reset()
{
	Wait();
	m_alloc->Reset();
	m_onEndEvent.clear();
	m_onEndEvent_regular.clear();
	return false;
}

bool DX12CommandUnit::UpdateBufferWithSysMem(BufferHandle dest, void * src, size_t size)
{
	BufferInfo* bufInfoPtr = m_device->PickupBuffer(dest);
	if (bufInfoPtr == nullptr) {
		FLOG("%s: Invalid buffer handle!\n", __FUNCTION__);
		return false;
	}
	/** 确认能否更新 */
	if (bufInfoPtr->size < size) {
		FLOG("%s: Size of target buffer is less than %zu, update failed!", __FUNCTION__, size);
		return false;
	}
	D3D12_HEAP_PROPERTIES prop;
	D3D12_HEAP_FLAGS flag;
	bufInfoPtr->ptr->GetHeapProperties(&prop, &flag);
	if (prop.Type != D3D12_HEAP_TYPE_UPLOAD) {
		FLOG("%s: CPU can't access this buffer!\n", __FUNCTION__);
		return false;
	}

	/** 直接map并修改 */
	void* memPtr = nullptr;
	bufInfoPtr->ptr->Map(0, nullptr, &memPtr);
	memcpy(memPtr, src, size);
	bufInfoPtr->ptr->Unmap(0, nullptr);

	return true;
}

bool DX12CommandUnit::ReadBackToSysMem(BufferHandle src, void * dest, size_t size)
{
	BufferInfo* bufInfoPtr = m_device->PickupBuffer(src);
	if (bufInfoPtr == nullptr) {
		FLOG("%s: Invalid buffer handle!\n", __FUNCTION__);
		return false;
	}
	D3D12_HEAP_PROPERTIES prop;
	D3D12_HEAP_FLAGS flag;
	bufInfoPtr->ptr->GetHeapProperties(&prop, &flag);
	if (bufInfoPtr->size > size) {
		FLOG("%s: Size of target buffer is less than %zu, update failed!", __FUNCTION__, size);
		return false;
	}
	if (prop.Type != D3D12_HEAP_TYPE_READBACK &&
		prop.Type != D3D12_HEAP_TYPE_UPLOAD) {
		FLOG("%s: CPU can't access this buffer\n", __FUNCTION__);
		return false;
	}

	void* memPtr = nullptr;
	bufInfoPtr->ptr->Map(0, nullptr, &memPtr);
	memcpy(dest, memPtr, size);
	bufInfoPtr->ptr->Unmap(0, nullptr);

	return true;
}

bool DX12CommandUnit::CopyBetweenGPUBuffer(BufferHandle src, BufferHandle dest, size_t srcOffset, size_t destOffset, size_t size)
{
	BufferInfo* srcBufInfoPtr = m_device->PickupBuffer(src);
	if (srcBufInfoPtr == nullptr) {
		FLOG("%s: Invalid source buffer handle\n", __FUNCTION__);
		return false;
	}
	BufferInfo* destBufInfoPtr = m_device->PickupBuffer(dest);
	if (destBufInfoPtr == nullptr) {
		FLOG("%s: Invalid dest buffer handle\n", __FUNCTION__);
		return false;
	}
	
	m_graphicCmdList->CopyBufferRegion(destBufInfoPtr->ptr, destOffset, srcBufInfoPtr->ptr, srcOffset, size);
	return true;
}

bool DX12CommandUnit::TransferState(BufferHandle buf, ResourceStates newState)
{
	BufferInfo* bufInfoPtr = m_device->PickupBuffer(buf);
	if (bufInfoPtr == nullptr) {
		FLOG("%s: Invalid buffer handle\n", __FUNCTION__);
		return false;
	}
	D3D12_HEAP_PROPERTIES prop;
	D3D12_HEAP_FLAGS flag;
	bufInfoPtr->ptr->GetHeapProperties(&prop, &flag);
	if (prop.Type == D3D12_HEAP_TYPE_UPLOAD || prop.Type == D3D12_HEAP_TYPE_READBACK) {
		MLOG("resource on upload/readback heap can't change state!\n");
		return true;
	}
	if (stateMatch(bufInfoPtr->state, newState)) return true;
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = bufInfoPtr->ptr;
	barrier.Transition.StateBefore = bufInfoPtr->state;
	barrier.Transition.StateAfter = ResourceStateToDX12ResourceState(newState);
	barrier.Transition.Subresource = 0;
	m_graphicCmdList->ResourceBarrier(1, &barrier);
	return true;
}

bool DX12CommandUnit::TransferState(Buffer* buf, ResourceStates newState) {
	DX12Buffer* dx12Buf = dynamic_cast<DX12Buffer*>(buf);
	assert(dx12Buf != nullptr);
	D3D12_HEAP_PROPERTIES prop;
	D3D12_HEAP_FLAGS flag;
	dx12Buf->GetResource()->GetHeapProperties(&prop, &flag);
	if (prop.Type == D3D12_HEAP_TYPE_UPLOAD || prop.Type == D3D12_HEAP_TYPE_READBACK) {
		MLOG("resource on upload/readback heap can't change state!\n");
		return true;
	}
	if (stateMatch(dx12Buf->GetResourceState(), newState)) return true;
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = dx12Buf->GetResource();
	barrier.Transition.StateBefore = dx12Buf->GetResourceState();
	barrier.Transition.StateAfter = ResourceStateToDX12ResourceState(newState);
	barrier.Transition.Subresource = 0;
	m_graphicCmdList->ResourceBarrier(1, &barrier);
	/** 修改对应的资源状态 */
	dx12Buf->GetResourceState() = ResourceStateToDX12ResourceState(newState);
	return true;
}

bool DX12CommandUnit::TransferState(TextureHandle tex, ResourceStates newState)
{
	TextureInfo* texInfoPtr = m_device->PickupTexture(tex);
	if (texInfoPtr == nullptr) {
		FLOG("%s: Invalid texture handle\n", __FUNCTION__);
		return false;
	}
	D3D12_HEAP_PROPERTIES prop;
	D3D12_HEAP_FLAGS flag;
	texInfoPtr->ptr->GetHeapProperties(&prop, &flag);
	if (prop.Type == D3D12_HEAP_TYPE_UPLOAD || prop.Type == D3D12_HEAP_TYPE_READBACK) {
		MLOG("resource on upload/readback heap can't change state!\n");
		return true;
	}
	if (stateMatch(texInfoPtr->state, newState)) return true;
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texInfoPtr->ptr;
	barrier.Transition.StateBefore = texInfoPtr->state;
	barrier.Transition.StateAfter = texInfoPtr->state = ResourceStateToDX12ResourceState(newState);
	/** TODO: 暂时只支持对子元素0的状态变更 */
	barrier.Transition.Subresource = 0;
	m_graphicCmdList->ResourceBarrier(1, &barrier);
	return true;
}

bool DX12CommandUnit::BindRenderTargetsAndDepthStencilBuffer(const std::vector<TextureHandle>& renderTargets, TextureHandle depthStencil)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles(renderTargets.size());
	for (size_t index = 0; index < rtvHandles.size(); ++index) {
		auto value = m_device->PickupRenderTarget(renderTargets[index]);
		if (!value) {
			FLOG("%s: Pick up render target failed!\n", __FUNCTION__);
			return false;
		}
		rtvHandles[index] = value.value();
	}
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	{
		auto value = m_device->PickupDepthStencilTarget(depthStencil);
		if (!value) {
			FLOG("%s: Pick up depth stencil failed!\n", __FUNCTION__);
			return false;
		}
		dsvHandle = value.value();
	}
	m_graphicCmdList->OMSetRenderTargets(rtvHandles.size(), rtvHandles.data(), false, &dsvHandle);
	return true;
}

bool DX12CommandUnit::ClearRenderTarget(TextureHandle renderTarget, const std::array<float, 4>& color)
{
	auto value = m_device->PickupRenderTarget(renderTarget);
	if (!value) {
		FLOG("%s: Pick up render target failed!\n", __FUNCTION__);
		return false;
	}
	m_graphicCmdList->ClearRenderTargetView(value.value(), color.data(), 0, NULL);
	return true;
}


END_NAME_SPACE
