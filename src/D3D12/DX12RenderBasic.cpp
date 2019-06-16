#include "DX12RenderBasic.h"

namespace UnknownVision {
	BufMgr::IndexType BufMgr::RequestBufferHandle() {
		std::lock_guard<std::mutex> lg(bm_mutex);
		BufferHandle res;
		if (freeIndices.empty()) {
			res = BufferHandle(buffers.size());
			buffers.push_back(SmartPTR<ID3D12Resource>());
		}
		else {
			res = freeIndices.back();
			freeIndices.pop_back();
		}
		return res;
	}

	void BufMgr::RevertBufferHandle(IndexType index) {
		std::lock_guard<std::mutex> lg(bm_mutex);
		freeIndices.push_back(index);
	}

	void BufMgr::SetBuffer(ElementType && newElement, IndexType index) {
		std::lock_guard<std::mutex> lg(bm_mutex);
		buffers[index.value()] = newElement;
	}

	const BufMgr::ElementType& BufMgr::operator[](const IndexType& index) const {
		std::lock_guard<std::mutex> lg(bm_mutex);
		return buffers[index.value()];
	}

	BufMgr::ElementType& BufMgr::operator[](const IndexType& index) {
		std::lock_guard<std::mutex> lg(bm_mutex);
		return buffers[index.value()];
	}

	ID3D12GraphicsCommandList* CommandListMgr::RequestCmdList() {
		std::lock_guard<std::mutex> lg(clm_mutex);
		ElementType ele;
		SmartPTR<ID3D12Device> dev;
		if (freeCmdLists.empty()) {
			allocator->GetDevice(IID_PPV_ARGS(&dev));
			dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&ele));
			cmdLists.push_back(ele);
		}
		else {
			ele = cmdLists[freeCmdLists.back()];
			freeCmdLists.pop_back();
			ele->Reset(allocator.Get(), nullptr);
		}
		return ele.Get();
	}


	void TransientResourceMgr::Store(ElementType& e) thread_safe {
		std::lock_guard<std::mutex> lg(trm_mutex);
		resources.push_back(e);
	}

	QueueProxy::QueueProxy(QueueProxy && qp) {
		queue.Swap(qp.queue);
		fence.Swap(qp.fence);
		fenceValue = qp.fenceValue;
		fenceEvent = qp.fenceEvent;
		qp.fenceEvent = NULL;
	}

	QueueProxy& QueueProxy::operator=(QueueProxy&& rhs) {
		queue.Swap(rhs.queue);
		fence.Swap(rhs.fence);
		fenceValue = rhs.fenceValue;
		fenceEvent = rhs.fenceEvent;
		rhs.fenceEvent = NULL;
		return *this;
	}

	void QueueProxy::Execute(uint32_t numCommandLists, ID3D12CommandList * const * ppCommandLists)
	{
		queue->ExecuteCommandLists(numCommandLists, ppCommandLists);
		queue->Signal(fence.Get(), ++fenceValue);
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}
