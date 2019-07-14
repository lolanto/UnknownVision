#pragma once

#include "../RenderSystem/RenderBasic.h"
#include "DX12Config.h"
#include "DX12ResourceManager.h"
#include <vector>
#include <memory>
#include <array>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>

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

/** 用于存放加载的shader字节码和reflect的属性 */
struct DX12Shader {
	D3D12_SHADER_BYTECODE byteCode;
};

class DX12RenderDevice : public RenderDevice {
	friend class DX12RenderBackend;
public:
	/** 单个句柄对应的实体的信息 */
	struct BufferInfo {
		BufferInfo(size_t size = 0, ID3D12Resource* ptr = nullptr,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
			: ptr(ptr), state(state), size(size) {}
		ID3D12Resource* ptr = nullptr;
		D3D12_RESOURCE_STATES state;
		const size_t size = 0;
	};

	struct TextureInfo {
		TextureInfo(ID3D12Resource* ptr = nullptr, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON, 
			uint32_t width = 0, uint32_t height = 0)
			: ptr(ptr), state(state), width(width), height(height) {}
		ID3D12Resource* ptr = nullptr;
		D3D12_RESOURCE_STATES state;
		const uint32_t width = 0, height = 0;
	};

	bool Initialize(std::string config) final;


	/** 允许额外的线程执行该函数，负责处理任务队列中的任务 */
	void Process() final;

	ID3D12Device* GetDevice() { return m_device.Get(); }

private:
	DX12RenderDevice(SmartPTR<ID3D12CommandQueue>& queue,
		SmartPTR<IDXGISwapChain1>& swapChain,
		SmartPTR<ID3D12Device>& device,
		uint32_t width, uint32_t height)
		: m_swapChain(swapChain), m_device(device), 
		m_backBuffers(decltype(m_backBuffers)(NUMBER_OF_BACK_BUFFERS)), m_curBackBufferIndex(0), RenderDevice(width, height),
		m_resourceManager(DX12ResourceManager(device.Get())) {
		SmartPTR<ID3D12Fence> fence;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	}

	const DX12RenderDevice& operator=(const DX12RenderDevice&) = delete;
	DX12RenderDevice(DX12RenderDevice&&) = delete;
	DX12RenderDevice(const DX12RenderDevice&) = delete;

	/** 从resource status转换出heapType和flags，供创建资源过程使用 */
	inline void fromResourceStatusToHeapTypeAndFlags(const ResourceStatus& status, D3D12_HEAP_TYPE& heapType, D3D12_RESOURCE_FLAGS& flags);

	void TEST_func(const Command&);

private:

	SmartPTR<IDXGISwapChain1> m_swapChain;
	SmartPTR<ID3D12Device> m_device;
	std::vector< SmartPTR<ID3D12Resource> > m_backBuffers; /**< 需要手动构建队列 */
	DX12ResourceManager m_resourceManager; /**< 资源管理器 */
	/** 当前可以写入的后台缓存的索引，每一次切换frame buffer的时候都需要更新该值
	 * 取值范围是[0, BACK_BUFFER_COUNT] */
	uint8_t m_curBackBufferIndex = 0; 

	std::map<BufferHandle, BufferInfo> m_buffers; /**< 所有缓冲区句柄的信息都在这里 */
	std::map<TextureHandle, TextureInfo> m_textures; /**< 所有纹理句柄信息都在这里 */
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
	ProgramDescriptor RequestProgram(ShaderNames shader, ProgramOptions opts, VertexAttributeDescs vtxAttDesc,
		bool usedIndex = true) final;
private:
	void analyseShader(const char* shaderName, ShaderType type);
private:
	bool m_isInitialized = false;
	SmartPTR<IDXGIFactory6> m_factory;
	std::vector<DX12RenderDevice*> m_devices;
	std::map<std::string, DX12Shader> m_shaders;
};
END_NAME_SPACE
