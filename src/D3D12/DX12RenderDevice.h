#pragma once
#include "DX12Config.h"
#include "CommandUnit/DX12CommandUnit.h"
#include "CommandUnit/DX12CommandListManager.h"
#include "DX12ResourceManager.h"
#include "DX12Pipeline.h"
#include "../RenderSystem/RenderDevice.h"
#include "../Utility/OptimisticLock/OptimisticLock.hpp"

BEG_NAME_SPACE

class DX12RenderDevice : public RenderDevice, public Uncopyable {
	friend class DX12RenderBackend;
private:
	class DX12SwapChainBufferWrapper : public GPUResource {
	public:
		void SetName(const wchar_t* name) override { m_pBackBuffer->SetName(name); }
		void* GetResource() override { if (m_pBackBuffer) return m_pBackBuffer; }
		DX12SwapChainBufferWrapper() = default;
		void Initialize(ID3D12Resource* res);
	public:
		ShaderResourceView* GetSRVPtr() { return &m_srv; }
		RenderTargetView* GetRTVPtr() { return &m_rtv; }
		virtual void Release() {};
	private:
		ID3D12Resource* m_pBackBuffer;
		DX12RenderTargetView m_rtv;
		DX12ShaderResourceView m_srv;
	};
/** 需要实现的虚函数 */
public:
	~DX12RenderDevice();
	bool Initialize(std::string config) final;
	
	CommandUnit* RequestCommandUnit(COMMAND_UNIT_TYPE type) final;
	/** 释放指令单元，所有已录制未执行的指令都将丢失 */
	void FreeCommandUnit(CommandUnit** cmdUnit);
	
	void UpdatePerFrame() override;

	bool Present() final;

	/** 构造GraphicsPipeline */
	virtual GraphicsPipelineObject* BuildGraphicsPipelineObject(
		VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rastOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList
	) final;

	GPUResource* BackBuffer() { return &m_swapChainResources[m_curBackBufferIndex]; }

/** 仅该子类拥有的函数 */
public:
	ID3D12Device* GetDevice() { return m_device.Get(); }
	DX12ResourceManager& ResourceManager() { return m_resourceManager; }
	DX12CommandListManager& CommandListManager() { return m_commandListManager; }
	DX12CommandUnitManager& CommandUnitManager() { return m_commandUnitManager; }
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupRenderTarget(TextureHandle tex) thread_safe;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupDepthStencilTarget(TextureHandle tex) thread_safe;
	void AddAsyncTask(std::function<bool()>&& task) { m_asyncTaskPerFrame.push_back(task); }
private:
	DX12RenderDevice(DX12RenderBackend& backend,
		SmartPTR<ID3D12CommandQueue>& queue,
		SmartPTR<IDXGISwapChain3>& swapChain,
		SmartPTR<ID3D12Device>& device,
		uint32_t width, uint32_t height);

	DX12RenderDevice(DX12RenderDevice&&) = delete;
private:
	DX12RenderBackend& m_backend;
	SmartPTR<IDXGISwapChain3> m_swapChain;
	SmartPTR<ID3D12Device> m_device;
	SmartPTR<ID3D12Resource> m_backBuffers[NUMBER_OF_BACK_BUFFERS]; /**< 需要手动构建队列 */
	DX12SwapChainBufferWrapper m_swapChainResources[NUMBER_OF_BACK_BUFFERS]; /**< 对backbuffer进行了封装 */
	/** 当前可以写入的后台缓存的索引，每一次切换frame buffer的时候都需要更新该值
	 * 取值范围是[0, BACK_BUFFER_COUNT] */
	uint8_t m_curBackBufferIndex;
	/** 处理各个backbuffer的queue指令对应的fence值
	 * 假如queue对应的Fence的complete值大于等于该值，证明对应的backbuffer已经处理完成 
	 * 初始情况下为0，代表没有任何指令在处理这些backbuffer */
	size_t m_backBufferFenceValues[NUMBER_OF_BACK_BUFFERS];

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	std::atomic_uint64_t m_totalFrame;

	DX12ResourceManager m_resourceManager; /**< 资源管理器 */
	DX12PipelineManager m_pipelineManager; /**< 负责创建，存储和检索Pipeline Object */
	DX12CommandListManager m_commandListManager; /**< 对DX12 CommandQueue和Allocator的封装 */
	DX12CommandUnitManager m_commandUnitManager;

	std::vector< std::function<bool()> > m_asyncTaskPerFrame; /**< 需要异步(延迟)执行的任务 */
};

END_NAME_SPACE
