#pragma once
#include "DX12Config.h"
#include "DX12CommandUnit.h"
#include "DX12ResourceManager.h"
#include "../RenderSystem/RenderDevice.h"

BEG_NAME_SPACE

class DX12RenderDevice : public RenderDevice {
	friend class DX12RenderBackend;
public:
	bool Initialize(std::string config) final;

	ID3D12Device* GetDevice() { return m_device.Get(); }

	CommandUnit& RequestCommandUnit(COMMAND_UNIT_TYPE type) final { return m_commandUnits[type]; }

	bool Present() final { return SUCCEEDED(m_swapChain->Present(1, 0)); }
	SpecialTextureResource CurrentBackBufferHandle() final { return SpecialTextureResource((uint8_t)DEFAULT_BACK_BUFFER + (uint8_t)m_swapChain->GetCurrentBackBufferIndex()); }

	DX12ResourceManager& ResourceManager() { return m_resourceManager; }

public:
	/** 仅该子类拥有的函数 */
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupRenderTarget(TextureHandle tex) thread_safe;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupDepthStencilTarget(TextureHandle tex) thread_safe;

private:
	DX12RenderDevice(DX12RenderBackend& backend,
		SmartPTR<ID3D12CommandQueue>& queue,
		SmartPTR<IDXGISwapChain3>& swapChain,
		SmartPTR<ID3D12Device>& device,
		uint32_t width, uint32_t height)
		: m_backend(backend), m_swapChain(swapChain), m_device(device),
		m_backBuffers(decltype(m_backBuffers)(NUMBER_OF_BACK_BUFFERS)), m_curBackBufferIndex(0), RenderDevice(width, height),
		m_resourceManager(DX12ResourceManager(device.Get())) {
		/** 获取各个descriptor heap的元素(步进)大小 */
		for (size_t heapType = 0; heapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++heapType) {
			m_descriptorHeapIncrementSize[heapType] = device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(heapType));
		}
		/** 初始化指令执行单位 */
		for (size_t commandUnitType = 0; commandUnitType < NUMBER_OF_COMMAND_UNIT_TYPE; ++commandUnitType) {
			m_commandUnits[commandUnitType] = DX12CommandUnit(this);
			if (commandUnitType == DEFAULT_COMMAND_UNIT) m_commandUnits[commandUnitType].Setup(queue);
			else m_commandUnits[commandUnitType].Setup(
				CommandUnitTypeToDX12CommandListType(
					static_cast<COMMAND_UNIT_TYPE>(commandUnitType)));
		}
		m_rtvHeapGen.back() = 0;
		m_dsvHeapGen.back() = 0;
		m_scuHeapManager.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_device.Get());
		m_samplerHeapManager.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_device.Get());
	}

	const DX12RenderDevice& operator=(const DX12RenderDevice&) = delete;
	DX12RenderDevice(DX12RenderDevice&&) = delete;
	DX12RenderDevice(const DX12RenderDevice&) = delete;

	/** 根据program descriptor 创建GraphicsPipeline state object */
	bool generateGraphicsPSO(const ProgramDescriptor& pmgDesc);
	/** 根据program descriptor 创建该program的root signature */
	bool generateGraphicsRootSignature(const ProgramInfo& pmgDesc, SmartPTR<ID3D12RootSignature>& rootSignature);
	bool generateGraphicsRootSignature(const std::vector<RootSignatureParameter>& parameters,
		const std::vector<std::pair<SamplerDescriptor, RootSignatureParameter>>& staticSamplers);
	bool generateBuffer(const BufferDescriptor& desc);
private:
	DX12RenderBackend& m_backend;
	SmartPTR<IDXGISwapChain3> m_swapChain;
	SmartPTR<ID3D12Device> m_device;
	std::vector< SmartPTR<ID3D12Resource> > m_backBuffers; /**< 需要手动构建队列 */
	DX12ResourceManager m_resourceManager; /**< 资源管理器 */
	/** 当前可以写入的后台缓存的索引，每一次切换frame buffer的时候都需要更新该值
	 * 取值范围是[0, BACK_BUFFER_COUNT] */
	uint8_t m_curBackBufferIndex = 0;

	SmartPTR<ID3D12DescriptorHeap> m_rtvHeap;
	SmartPTR<ID3D12DescriptorHeap> m_dsvHeap;
	DX12DescriptorHeapManager m_scuHeapManager;
	DX12DescriptorHeapManager m_samplerHeapManager;
	UINT m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	/** 最后一个元素存储当前空闲的位置索引 */
	mutable OptimisticLock m_rtvHeapGenLock;
	std::array<uint32_t, NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP + 1> m_rtvHeapGen;

	mutable OptimisticLock m_dsvHeapGenLock;
	std::array<uint32_t, NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP + 1> m_dsvHeapGen;

	DX12CommandUnit m_commandUnits[NUMBER_OF_COMMAND_UNIT_TYPE]; /**< 包含所有可用的指令单元 */

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	mutable OptimisticLock m_bufferLock;
	std::map<BufferHandle, BufferInfo> m_buffers; /**< 所有缓冲区句柄的信息都在这里 */
	mutable OptimisticLock m_textureLock;
	std::map<TextureHandle, TextureInfo> m_textures; /**< 所有纹理句柄信息都在这里 */
	//mutable OptimisticLock m_samplerLock;
	//std::map<SamplerHandle, SamplerInfo> m_samplers; /**< 所有采样方式信息都在这里 */
	mutable OptimisticLock m_programLock;
	std::map<ProgramHandle, ProgramInfo> m_programs;

	std::atomic_uint64_t m_totalFrame;
};

END_NAME_SPACE
