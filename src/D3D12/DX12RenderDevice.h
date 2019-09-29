#pragma once
#include "DX12Config.h"
#include "DX12CommandUnit.h"
#include "DX12ResourceManager.h"
#include "DX12Pipeline.h"
#include "../RenderSystem/RenderDevice.h"
#include "../Utility/OptimisticLock/OptimisticLock.hpp"

BEG_NAME_SPACE

class DX12RenderDevice : public RenderDevice {
	friend class DX12RenderBackend;
public:
	bool Initialize(std::string config) final;

	CommandUnit& RequestCommandUnit(COMMAND_UNIT_TYPE type) final { return m_commandUnits[type]; }

	bool Present() final { return SUCCEEDED(m_swapChain->Present(1, 0)); }

	/** 构造GraphicsPipeline */
	virtual GraphicsPipelineObject* BuildGraphicsPipelineObject(
		VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rastOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList
	) final;

/** 仅该子类拥有的函数 */
public:
	ID3D12Device* GetDevice() { return m_device.Get(); }
	SpecialTextureResource CurrentBackBufferHandle() final { return SpecialTextureResource((uint8_t)DEFAULT_BACK_BUFFER + (uint8_t)m_swapChain->GetCurrentBackBufferIndex()); }
	DX12ResourceManager& ResourceManager() { return m_resourceManager; }
	
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupRenderTarget(TextureHandle tex) thread_safe;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupDepthStencilTarget(TextureHandle tex) thread_safe;

private:
	DX12RenderDevice(DX12RenderBackend& backend,
		SmartPTR<ID3D12CommandQueue>& queue,
		SmartPTR<IDXGISwapChain3>& swapChain,
		SmartPTR<ID3D12Device>& device,
		uint32_t width, uint32_t height);

	const DX12RenderDevice& operator=(const DX12RenderDevice&) = delete;
	DX12RenderDevice(DX12RenderDevice&&) = delete;
	DX12RenderDevice(const DX12RenderDevice&) = delete;
public:
	/** TODO: 提供资源创建的接口 */
private:
	DX12RenderBackend& m_backend;
	SmartPTR<IDXGISwapChain3> m_swapChain;
	SmartPTR<ID3D12Device> m_device;
	std::vector< SmartPTR<ID3D12Resource> > m_backBuffers; /**< 需要手动构建队列 */
	
	/** 当前可以写入的后台缓存的索引，每一次切换frame buffer的时候都需要更新该值
	 * 取值范围是[0, BACK_BUFFER_COUNT] */
	uint8_t m_curBackBufferIndex = 0;

	DX12CommandUnit m_commandUnits[NUMBER_OF_COMMAND_UNIT_TYPE]; /**< 包含所有可用的指令单元 */

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	std::atomic_uint64_t m_totalFrame;

	DX12ResourceManager m_resourceManager; /**< 资源管理器 */
	DX12PipelineManager m_pipelineManager; /**< 负责创建，存储和检索Pipeline Object */
};

END_NAME_SPACE
