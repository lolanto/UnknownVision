#pragma once

#include "../RenderSystem/RenderBasic.h"
#include "DX12Config.h"
#include <vector>
#include <memory>
#include <array>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <deque>

#define MemoryManagementStrategy NoMemMng

BEG_NAME_SPACE

class DX12RenderBackend;
class DX12RenderDevice;


enum CmdQueueType {
	GraphicQue = 0,
	CmdQueueCount
};


class BufMgr {
public:
	using ElementType = SmartPTR<ID3D12Resource>;
	using IndexType = BufferHandle;
private:
	std::deque<ElementType> buffers; /**< 当前正在管理的所有buffer资源 */
	std::vector<IndexType> freeIndices; /**< 空闲buffer句柄 */
	mutable std::mutex bm_mutex;
public:
	IndexType RequestBufferHandle() thread_safe;
	void RevertBufferHandle(IndexType index) thread_safe;
	void SetBuffer(ElementType&& newElement, IndexType index) thread_safe;
	const ElementType& operator[](const IndexType& index) thread_safe_const;
	ElementType& operator[](const IndexType& index) thread_safe;
};


class TransientResourceMgr {
public:
	using ElementType = SmartPTR<ID3D12Resource>;
private:
	std::vector<ElementType> resources;
	std::mutex trm_mutex;
public:
	void Store(ElementType& e) thread_safe;
	void Reset() { resources.swap(std::vector<ElementType>()); }
};

class CommandListMgr {
public:
	using ElementType = SmartPTR<ID3D12GraphicsCommandList>;
	CommandListMgr(ID3D12Device* dev) { dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)); }
private:
	SmartPTR<ID3D12CommandAllocator> allocator;
	std::vector<ElementType> cmdLists;
	std::vector<uint8_t> freeCmdLists;
	mutable std::mutex clm_mutex;
public:
	ID3D12GraphicsCommandList* RequestCmdList() thread_safe;
	const std::vector<ElementType>& RequestWholeList() const { return cmdLists; }
	void SimpleRest() {
		freeCmdLists.resize(cmdLists.size());
		for (uint8_t i = 0; i < freeCmdLists.size(); ++i)
			freeCmdLists[i] = i;
	}
	void DeepRest() {
		cmdLists.clear();
		freeCmdLists.clear();
		allocator->Reset();
	}
};

class QueueProxy {
public:
	QueueProxy(SmartPTR<ID3D12CommandQueue>& que,
		SmartPTR<ID3D12Fence>& fen)
		: queue(que), fence(fen) {
		fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		fenceValue = 0;
	}
	QueueProxy(const QueueProxy&) = delete;
	QueueProxy& operator=(const QueueProxy&) = delete;
	QueueProxy(QueueProxy&& qp);
	QueueProxy& operator=(QueueProxy&& rhs);
	QueueProxy() : fenceEvent(NULL), fenceValue(0) {}
	~QueueProxy() { if (fenceEvent) CloseHandle(fenceEvent); }
	ID3D12Fence* GetFence() { return fence.Get(); }
	void Execute(uint32_t numCommandLists, ID3D12CommandList* const* ppCommandLists);
	void Wait(ID3D12Fence* fen, uint64_t value) { queue->Wait(fen, value); }
private:
	SmartPTR<ID3D12CommandQueue> queue;
	SmartPTR<ID3D12Fence> fence;
	HANDLE fenceEvent;
	uint64_t fenceValue;
};

class DX12RenderDevice : public RenderDevice {
	friend class DX12RenderBackend;
public:

	bool Initialize(std::string config) final;
	BufferHandle RequestBuffer(size_t size, ResourceUsage usage, ResourceFlag flag, bool manual) final thread_safe;

	/** 待实现 */
	std::function<void()> Process(Task&) thread_safe final {
		return []() {};
	}

	ID3D12Device* GetDevice() { return m_device.Get(); }

private:
	DX12RenderDevice(SmartPTR<ID3D12CommandQueue>& queue,
		SmartPTR<IDXGISwapChain1>& swapChain,
		SmartPTR<ID3D12Device>& device)
		: m_swapChain(swapChain), m_device(device) {
		SmartPTR<ID3D12Fence> fence;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	}

	const DX12RenderDevice& operator=(const DX12RenderDevice&) = delete;
	DX12RenderDevice(DX12RenderDevice&&) = delete;
	DX12RenderDevice(const DX12RenderDevice&) = delete;
private:

	SmartPTR<IDXGISwapChain1> m_swapChain;
	SmartPTR<ID3D12Device> m_device;

	std::atomic_uint64_t m_totalFrame;
};

class DX12RenderBackend : public RenderBackend {
public:
	~DX12RenderBackend() {
		for (auto devPtr : m_devices)
			delete devPtr;
	}
public:
	bool Initialize() final;
	bool isInitialized() const final { return m_isInitialized; }
	RenderDevice* CreateDevice(void* parameters) final;
private:
	bool m_isInitialized = false;
	SmartPTR<IDXGIFactory6> m_factory;
	std::vector<DX12RenderDevice*> m_devices;
};
END_NAME_SPACE
