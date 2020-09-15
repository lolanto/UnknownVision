#pragma once
#include "DX12Config.h"
#include "DX12CommandUnit.h"
#include "DX12ResourceManager.h"
#include "DX12Pipeline.h"
#include "../GraphicsInterface/RenderDevice.h"
#include <unordered_map>

BEG_NAME_SPACE
class DX12RenderBackend;

class DX12RenderDevice : public RenderDevice, public Uncopyable {
	friend class DX12RenderBackend;
private:
	class DX12SwapChainBufferWrapper : public Texture2D {
	public:
		void SetName(const wchar_t* name) override final { m_pBackBuffer->SetName(name); }
		void* GetResource() override final { 
			if (m_pBackBuffer == nullptr) LOG_ERROR("haven't initialize swap chain wrapper");
			return m_pBackBuffer;
		}
		DX12SwapChainBufferWrapper() = default;
		/** Note: 必须保证this和handle在Device中关联起来了 */
		void Initialize(ID3D12Resource* res, ID3D12Device* pDev, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	public:
		bool Avaliable() const final { return true; }
		virtual void Release() {}; /**< 资源是由swap chain管理的，所以不支持手动Release */
	private:
		ID3D12Resource* m_pBackBuffer;
	};
/** 需要实现的虚函数 */
public:
	~DX12RenderDevice();
	/** Note: 仅将那些提供可配置选项的对象创建过程置于此 */
	bool Initialize(std::string config) final;
	virtual void WaitForCommandExecutionComplete(size_t fenceValue, COMMAND_UNIT_TYPE type) override final;
	virtual bool QueryCommandExecutionState(size_t fenceValue, COMMAND_UNIT_TYPE type) override final;
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
	Texture2D* BackBuffer(size_t idx = SIZE_MAX) override final { if (idx == SIZE_MAX) return &m_swapChainResources[m_curBackBufferIndex]; else return &m_swapChainResources[idx]; }
	Texture2D* DepthStencilBuffer(size_t idx = SIZE_MAX) override final { if (idx == SIZE_MAX) return m_depthStencilBuffers[m_curBackBufferIndex].get(); else return m_depthStencilBuffers[idx].get(); }
	Buffer* CreateBuffer(size_t capacity, size_t elementStride, ResourceStatus status) override final;
	virtual Texture2D* CreateTexture2D(size_t width, size_t height, size_t miplevels, size_t arrSize, ElementFormatType format, ResourceStatus status) override final;
	bool WriteToBuffer(void* pSrc, Buffer* pDest, size_t srcSize, size_t destOffset, CommandUnit* cmdUnit) override final;
	bool WriteToTexture2D(const std::vector<ImageDesc>& srcDesc, Texture2D* pDest, CommandUnit* cmdUnit) override final;
	bool WriteToTexture2DArr(const std::vector<std::vector<ImageDesc>>& srcDesc, Texture2D* pDest, CommandUnit* cmdUnit) override final;
	bool ReadFromTexture2D(std::vector<uint8_t>& output, Texture2D* pSrc, CommandUnit* cmdUnit) override final;
	virtual BindingBoard* RequestBindingBoard(size_t numOfSlots, COMMAND_UNIT_TYPE type) override final;
/** 仅该子类拥有的函数 */
public:
	ID3D12Device* GetDevice() { return m_device.Get(); }
	DX12ResourceManager& ResourceManager() { return m_resourceManager; }
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupRenderTarget(GPUResource* tex) thread_safe;
	std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> PickupDepthStencilTarget(GPUResource* tex) thread_safe;
	AllocateRange RequestDescriptorBlocks(size_t capacity, COMMAND_UNIT_TYPE type);
	void ReleaseDescriptorBlocks(AllocateRange range, size_t fenceValue, COMMAND_UNIT_TYPE type);
	ID3D12DescriptorHeap* GetDescriptorHeap(COMMAND_UNIT_TYPE type);
	void ForDebug(Texture2D* tex);
private:
	/** 尽量避免构造函数抛出异常 */
	DX12RenderDevice(DX12RenderBackend& backend,
		SmartPTR<ID3D12CommandQueue>& queue,
		SmartPTR<IDXGISwapChain3>& swapChain,
		SmartPTR<ID3D12Device>& device,
		SmartPTR<IDXGIAdapter4>& adapter,
		uint32_t width, uint32_t height, size_t node);

	DX12RenderDevice(DX12RenderDevice&&) = delete;
private:
	DX12RenderBackend& m_backend;
	SmartPTR<IDXGIAdapter4> m_adapter;
	SmartPTR<IDXGISwapChain3> m_swapChain;
	SmartPTR<ID3D12Device> m_device;
	SmartPTR<ID3D12Resource> m_backBuffers[NUMBER_OF_BACK_BUFFERS]; /**< 需要手动构建队列 */
	DX12SwapChainBufferWrapper m_swapChainResources[NUMBER_OF_BACK_BUFFERS]; /**< 对backbuffer进行了封装 */
	std::unique_ptr<Texture2D> m_depthStencilBuffers[NUMBER_OF_BACK_BUFFERS];
	/** 当前可以写入的后台缓存的索引，每一次切换frame buffer的时候都需要更新该值
	 * 取值范围是[0, BACK_BUFFER_COUNT] */
	uint8_t m_curBackBufferIndex;
	/** 处理各个backbuffer的queue指令对应的fence值
	 * 假如queue对应的Fence的complete值大于等于该值，证明对应的backbuffer已经处理完成 
	 * 初始情况下为0，代表没有任何指令在处理这些backbuffer */
	size_t m_backBufferFenceValues[NUMBER_OF_BACK_BUFFERS];

	std::atomic_uint64_t m_totalFrame;

	DX12ResourceManager m_resourceManager; /**< 资源管理器 */
	DX12PipelineManager m_pipelineManager; /**< 负责创建，存储和检索Pipeline Object */

	DX12CommandUnitManager m_commandUnitManager_graphics;
	LocalDynamicDX12DescriptorHeap m_graphics_srv_cbv_uav_descriptorHeap;

	DiscretePermanentDX12DescriptorHeap m_rtvDescriptorHeap; /**< 存储RTV handle */

	std::mutex m_rtvPointerToHandle_mutex;
	std::unordered_map<GPUResource*, D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvPointerToHandle;

	DiscretePermanentDX12DescriptorHeap m_dsvDescriptorHeap; /**< 存储长期的DSV handle */
	std::mutex m_dsvPointerToHandle_mutex;
	std::unordered_map<GPUResource*, D3D12_CPU_DESCRIPTOR_HANDLE> m_dsvPointerToHandle;

};

END_NAME_SPACE
