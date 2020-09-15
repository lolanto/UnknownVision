#include "DX12RenderDevice.h"
#include "GPUResource/DX12GPUResource.h"
#include "DX12BindingBoard.h"
#include "../Image/DXImage.h"
#include <cassert>
#include <optional>
#include <iostream>

#define XifFailed(function, behavior) if (FAILED(function)) behavior

BEG_NAME_SPACE

UINT GDescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

void DX12RenderDevice::DX12SwapChainBufferWrapper::Initialize(
	ID3D12Resource * res, ID3D12Device * pDev, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Format = BACK_BUFFER_FORMAT;
	desc.Texture2D.MipSlice = 0;
	desc.Texture2D.PlaneSlice = 0;
	pDev->CreateRenderTargetView(res, &desc, handle);
	m_pBackBuffer = res;
	/** Note: 仅限在swapchain上的backbuffer初始状态 */
	m_state = DX12ResourceStateToResourceState(D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_PRESENT);
	m_status = ResourceStatus(RESOURCE_USAGE_RENDER_TARGET | RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY);
}

DX12RenderDevice::DX12RenderDevice(DX12RenderBackend& backend,
	SmartPTR<ID3D12CommandQueue>& queue,
	SmartPTR<IDXGISwapChain3>& swapChain,
	SmartPTR<ID3D12Device>& device,
	uint32_t width, uint32_t height, size_t node)
	: m_backend(backend), m_swapChain(swapChain), m_device(device),
	m_curBackBufferIndex(0), RenderDevice(width, height, node),
	m_resourceManager(DX12ResourceManager(device.Get())), m_pipelineManager(device.Get()),
	m_commandUnitManager_graphics(DEFAULT_COMMAND_UNIT){
	/** 计算当前环境下Descriptor的步进大小 */
	for (uint8_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		GDescriptorHandleIncrementSize[i] = 
		m_device->GetDescriptorHandleIncrementSize(
			static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

	/** Note: Fence创建失败/EventHandle创建失败可能会抛出异常 */
	m_commandUnitManager_graphics.Create(this, queue);

	m_curBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	for (auto& value : m_backBufferFenceValues) value = 0;
}

void DX12RenderDevice::WaitForCommandExecutionComplete(size_t fenceValue, COMMAND_UNIT_TYPE type)
{
	switch (type)
	{
	case UnknownVision::DEFAULT_COMMAND_UNIT:
		m_commandUnitManager_graphics.WaitForFence(fenceValue);
		break;
	default:
		assert(false);
		break;
	}
}

bool DX12RenderDevice::QueryCommandExecutionState(size_t fenceValue, COMMAND_UNIT_TYPE type)
{
	switch (type) {
	case DEFAULT_COMMAND_UNIT:
		return m_commandUnitManager_graphics.IsFenceComplete(fenceValue);
	default:
		assert(false);
	}
	return false;
}

void DX12RenderDevice::FreeCommandUnit(CommandUnit ** cmdUnit)
{
	DX12CommandUnit** cu = reinterpret_cast<DX12CommandUnit**>(cmdUnit);
	switch ((*cu)->CommandUnitType) {
	case DEFAULT_COMMAND_UNIT:
		m_commandUnitManager_graphics.FreeCommandUnit(cu);
		break;
	default:
		assert(false);
	}
	(*cmdUnit) = nullptr;
}

void DX12RenderDevice::UpdatePerFrame()
{
}

bool DX12RenderDevice::Present()
{
	assert(SUCCEEDED(m_swapChain->Present(1, 0)));
	/** 更新当前backbuffer的fence值，该fence值代表了当前backbuffer已经present完成
	 * 以备将来确认backbuffer是否可用 */
	m_backBufferFenceValues[m_curBackBufferIndex] = m_commandUnitManager_graphics.IncrementFence();

	uint64_t nextBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
	/** 等待下一个backbuffer可用 */
	m_commandUnitManager_graphics.WaitForFence(m_backBufferFenceValues[nextBackBuffer]);
	m_curBackBufferIndex = nextBackBuffer;
	return true;
}

CommandUnit * DX12RenderDevice::RequestCommandUnit(COMMAND_UNIT_TYPE type)
{
	CommandUnit* cu = nullptr;
	switch (type) {
	case DEFAULT_COMMAND_UNIT:
		cu = m_commandUnitManager_graphics.AllocateCommandUnit();
		break;
	}
	assert(cu != nullptr);
	return cu;
}

DX12RenderDevice::~DX12RenderDevice()
{
	/** 需要等待所有相关联的GPU指令执行完成后才能退出 */
	//m_commandListManager.IdleGPU();
	m_commandUnitManager_graphics.WaitForIdle();
}

bool DX12RenderDevice::Initialize(std::string config)
{
	/** 初始化必须的组件 */
	assert(m_resourceManager.Initialize());
	assert(m_rtvDescriptorHeap.Initialize(m_device.Get(), NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	assert(m_dsvDescriptorHeap.Initialize(m_device.Get(), NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
	assert(m_graphics_srv_cbv_uav_descriptorHeap.Initialize(m_device.Get(), NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	/** 构建特殊资源 */
	{
		std::lock_guard<std::mutex> lg(m_rtvPointerToHandle_mutex);
		for (uint8_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx) {
			XifFailed(m_swapChain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx])), {
				FLOG("%s: Create Back buffer %d FAILED!\n", __FUNCTION__, idx);
				return false;
				});
			auto handle = m_rtvDescriptorHeap.RequestBlock();
			m_swapChainResources[idx].Initialize(m_backBuffers[idx].Get(), m_device.Get(), handle);
			m_rtvPointerToHandle.insert({ &m_swapChainResources[idx], handle });
			m_backBuffers[idx]->SetName(L"BackBuffer");
		}
	}

	RenderDevice::Initialize(config);
	return true;
}


std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> DX12RenderDevice::PickupRenderTarget(GPUResource* tex) thread_safe {
	std::lock_guard<std::mutex> lg(m_rtvPointerToHandle_mutex);
	auto iter = m_rtvPointerToHandle.find(tex);
	if (iter == m_rtvPointerToHandle.end()) {
		auto allocateHandle = m_rtvDescriptorHeap.RequestBlock();
		D3D12_RENDER_TARGET_VIEW_DESC desc;
		if (tex->Type() == GPU_RESOURCE_TYPE_TEXTURE2D) {
			auto ptr = dynamic_cast<DX12Texture2D*>(tex);
			assert(ptr != nullptr);
			desc = ptr->GetRenderTargetView(0);
			m_device->CreateRenderTargetView(
				reinterpret_cast<ID3D12Resource*>(tex->GetResource()),
				&desc, allocateHandle);
		}
		else {
			assert(false);
		}
		m_rtvPointerToHandle.insert({ tex, allocateHandle });
		return allocateHandle;
	}
	else {
		return iter->second;
	}
}

AllocateRange DX12RenderDevice::RequestDescriptorBlocks(size_t capacity, COMMAND_UNIT_TYPE type)
{
	AllocateRange range = AllocateRange::INVALID();
	if (type == DEFAULT_COMMAND_UNIT) {
		size_t lastFenceValue = m_commandUnitManager_graphics.LastCompletedFence();
		range = m_graphics_srv_cbv_uav_descriptorHeap.RequestBlock(capacity, lastFenceValue);
	}
	else {
		assert(false);
	}
	assert(range.Valid());
	return range;
}

void DX12RenderDevice::ReleaseDescriptorBlocks(AllocateRange range, size_t fenceValue, COMMAND_UNIT_TYPE type)
{
	if (type == DEFAULT_COMMAND_UNIT) {
		m_graphics_srv_cbv_uav_descriptorHeap.ReleaseBlock(range, fenceValue);
	}
	else {
		assert(false);
	}
}

ID3D12DescriptorHeap* DX12RenderDevice::GetDescriptorHeap(COMMAND_UNIT_TYPE type)
{
	switch (type) {
	case DEFAULT_COMMAND_UNIT:
		return m_graphics_srv_cbv_uav_descriptorHeap.GetHeap();
		break;
	default:
		return nullptr;
	}
}


GraphicsPipelineObject* DX12RenderDevice::BuildGraphicsPipelineObject(
	VertexShader* vs, PixelShader* ps,
	RasterizeOptionsFunc rastOpt, OutputStageOptionsFunc outputOpt,
	VertexAttributesFunc vtxAttribList) {
	/** 构造GraphicsPipeline不能缺少VS和PS，必须提前进行初始化 */
	if (vs->GetHandle() == ShaderHandle::InvalidIndex() ||
		ps->GetHandle() == ShaderHandle::InvalidIndex()) {
		return nullptr;
	}
	{
		/** 下面的PSO只是一个临时的容器而已 */
		DX12GraphicsPipelineObject graphicsPSO(vs, ps, rastOpt, outputOpt, vtxAttribList);
		auto dx12vs = GShaderManager[vs->GetHandle()];
		auto dx12ps = GShaderManager[ps->GetHandle()];
		return m_pipelineManager.Build(graphicsPSO, dx12vs, dx12ps);
	}
}


Buffer* DX12RenderDevice::CreateBuffer(size_t capacity, size_t elementStride, ResourceStatus status)
{
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	if (status.isFrequently() || status.isOnce()) heapType = D3D12_HEAP_TYPE_UPLOAD;
	else if (status.isReadBack()) heapType = D3D12_HEAP_TYPE_READBACK;
	size_t bufferSizeInBytes = capacity * elementStride;
	if (status.canBeConstantBuffer()) { /**< DX12规定资源作为CBV时，buffer的尺寸必须是256的倍数 */
		bufferSizeInBytes = static_cast<size_t>(
			std::ceil(static_cast<float>(bufferSizeInBytes) / 256.0f) * 256.0f
			);
	}
	auto [pRes, state] = m_resourceManager.RequestBuffer(bufferSizeInBytes,
		ResourceStatusToResourceFlag(status),
		heapType);
	if (pRes == nullptr) return nullptr;
	DX12Buffer* newBuf = new DX12Buffer();
	newBuf->m_status = status;
	newBuf->m_state = DX12ResourceStateToResourceState(state);
	newBuf->m_pResMgr = &m_resourceManager;
	newBuf->m_pBuffer = pRes;
	newBuf->m_capacity = capacity;
	newBuf->m_strideInBytes = elementStride;
	return newBuf;
}

Texture2D* DX12RenderDevice::CreateTexture2D(size_t width, size_t height, size_t miplevels, ElementFormatType format, ResourceStatus status)
{
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	if (status.isFrequently() || status.isOnce()) heapType = D3D12_HEAP_TYPE_UPLOAD;
	else if (status.isReadBack()) heapType = D3D12_HEAP_TYPE_READBACK;
	auto [pRes, state] = m_resourceManager.RequestTexture(
		width, height, 0,
		ElementFormatToDXGIFormat(format),
		ResourceStatusToResourceFlag(status),
		miplevels, false
	);
	if (pRes == nullptr) return nullptr;
	DX12Texture2D* newTex2D = new DX12Texture2D();
	newTex2D->m_width = width;
	newTex2D->m_height = height;
	newTex2D->m_miplevels = miplevels;
	newTex2D->m_pResMgr = &m_resourceManager;
	newTex2D->m_pTexture = pRes;
	newTex2D->m_format = format;
	newTex2D->m_state = DX12ResourceStateToResourceState(state);
	newTex2D->m_status = status;
	return newTex2D;
}

bool DX12RenderDevice::WriteToBuffer(void* pSrc, Buffer* pDest, size_t srcSize, size_t destOffset, CommandUnit* cmdUnit)
{
	DX12Buffer* pbuf = dynamic_cast<DX12Buffer*>(pDest);
	ID3D12Resource* pRes = reinterpret_cast<ID3D12Resource*>(pbuf->GetResource());
	DX12CommandUnit* pUnit = dynamic_cast<DX12CommandUnit*>(cmdUnit);
	assert(pDest->MemFootprint() >= srcSize + destOffset);
	if (pDest->m_status.isStably() == false) {
		uint8_t* pData = nullptr;
		pRes->Map(0, nullptr, (void**)&pData);
		pData += destOffset;
		memcpy(pData, pSrc, srcSize);
		pRes->Unmap(0, nullptr);
	}
	else {
		auto [pIntermediateRes, state] = m_resourceManager.RequestBuffer(srcSize, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
		assert(pIntermediateRes != nullptr && state == D3D12_RESOURCE_STATE_GENERIC_READ);
		void* pData = nullptr;
		pIntermediateRes->Map(0, nullptr, &pData);
		memcpy(pData, pSrc, srcSize);
		pIntermediateRes->Unmap(0, nullptr);
		if (destOffset == 0) {
			pUnit->m_graphicsCmdList->CopyResource(pRes, pIntermediateRes);
		}
		else {
			pUnit->m_graphicsCmdList->CopyBufferRegion(pRes, destOffset, pIntermediateRes, 0, srcSize);
		}
		cmdUnit->Flush(true);
		m_resourceManager.ReleaseResource(pIntermediateRes);
	}
	return true;
}

bool DX12RenderDevice::WriteToTexture2D(Image* pSrc, Texture2D* pDest, CommandUnit* cmdUnit)
{
	DX12CommandUnit* pUnit = dynamic_cast<DX12CommandUnit*>(cmdUnit);
	DX12Texture2D* pTex = dynamic_cast<DX12Texture2D*>(pDest);
	ID3D12Resource* pRes = reinterpret_cast<ID3D12Resource*>(pTex->GetResource());
	DXImage* pdxSrc = dynamic_cast<DXImage*>(pSrc);
	DirectX::TexMetadata metadata = pdxSrc->m_image.GetMetadata();
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	/** 利用辅助函数填充D3D12_SUBRESOURCE_DATA结构体信息 */
	assert(SUCCEEDED(
		DirectX::PrepareUpload(m_device.Get(), pdxSrc->m_image.GetImages(), pdxSrc->m_image.GetImageCount(), metadata, subresources)
	));
	assert(subresources.size() <= pDest->MipLevels()); /**< 即将写入的子资源数量必须能够被pDest容纳 */
	size_t intermediateBufferSize = GetRequiredIntermediateSize(pRes, 0, subresources.size());
	auto [pIntermediateRes, state] = m_resourceManager.RequestBuffer(intermediateBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
	assert(pIntermediateRes != nullptr && state == D3D12_RESOURCE_STATE_GENERIC_READ);
	assert(UpdateSubresources(pUnit->m_graphicsCmdList.Get(), pRes, pIntermediateRes, 0, 0, subresources.size(), subresources.data())
		!= 0);
	cmdUnit->Flush(true);
	m_resourceManager.ReleaseResource(pIntermediateRes);
	return true;
}

BindingBoard* DX12RenderDevice::RequestBindingBoard(size_t numOfSlots, COMMAND_UNIT_TYPE type)
{
	DX12BindingBoard* ptr = new DX12BindingBoard();
	ptr->Initialize(numOfSlots, this, type);
	return ptr;
}

void DX12RenderDevice::ForDebug(Texture2D* tex)
{
	DX12Texture2D* p = dynamic_cast<DX12Texture2D*>(tex);
	auto&& view = p->GetShaderResourceView();
	m_device->CreateShaderResourceView(
		reinterpret_cast<ID3D12Resource*>(p->GetResource()),
		&view,
		m_graphics_srv_cbv_uav_descriptorHeap.GetCPUHandle(0)
	);
}

END_NAME_SPACE
