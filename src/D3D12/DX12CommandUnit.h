#pragma once
#include "DX12Config.h"
#include "DX12Helpers.h"
#include "../RenderSystem/CommandUnit.h"

BEG_NAME_SPACE

class DX12RenderDevice;
class Buffer;

enum EventTime {
	EVENT_TIME_ON_END = 0,
	EVENT_TIME_NUM
};

class DX12CommandUnit : public CommandUnit {
public:
	bool Active() final;
	bool Fetch() final;
	bool FetchAndPresent() final;
	bool Wait() final;
	bool RegisterEvent(std::function<void()> e, EventTime time, bool isRegularEvent = false);
	bool Reset() final;
	bool UpdateBufferWithSysMem(BufferHandle dest, void* src, size_t size) final;
	bool ReadBackToSysMem(BufferHandle src, void* dest, size_t size) final;
	bool CopyBetweenGPUBuffer(BufferHandle src, BufferHandle dest, size_t srcOffset, size_t destOffset, size_t size) final;
	bool TransferState(BufferHandle buf, ResourceStates newState) final;
	bool TransferState(TextureHandle tex, ResourceStates newState) final;
	bool TransferState(Buffer* buf, ResourceStates newState) final;
	bool BindRenderTargetsAndDepthStencilBuffer(const std::vector<TextureHandle>& renderTargets, TextureHandle depthStencil) final;
	bool ClearRenderTarget(TextureHandle renderTarget, const std::array<float, 4>& color) final;
public:
	DX12CommandUnit(DX12RenderDevice* device = nullptr) : m_device(device),
		m_fenceEvent(INVALID_HANDLE_VALUE), m_nextFenceValue(1), m_executing(false) {}
	~DX12CommandUnit() {
		if (INVALID_HANDLE_VALUE != m_fenceEvent) CloseHandle(m_fenceEvent);
	}
	void Setup(SmartPTR<ID3D12CommandQueue> queue);
	void Setup(D3D12_COMMAND_LIST_TYPE);
private:
	void initializeFence();
	void initializeCommandAllocAndList();
	bool stateMatch(D3D12_RESOURCE_STATES oriStates, ResourceStates newStates) {
		if (newStates == RESOURCE_STATE_PRESENT) {
			if (oriStates != 0) return false;
		}
		if ((oriStates & ResourceStateToDX12ResourceState(newStates)) == ResourceStateToDX12ResourceState(newStates))
			return true;
		return false;
	}
private:
	DX12RenderDevice* m_device;
	SmartPTR<ID3D12CommandQueue> m_queue;
	SmartPTR<ID3D12CommandAllocator> m_alloc;
	/** TODO: 暂时只支持一个graphic command list */
	SmartPTR<ID3D12GraphicsCommandList> m_graphicCmdList;
	SmartPTR<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;
	uint64_t m_nextFenceValue;
	uint64_t m_lastFenceValue;
	bool m_executing;
	
	std::vector<std::function<void()>> m_onEndEvent;
	std::vector<std::function<void()>> m_onEndEvent_regular;
};

END_NAME_SPACE
