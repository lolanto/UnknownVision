#include "DX12RenderDevice.h"
#include "DX12GPUResource.h"
#include "DX12BindingBoard.h"
#include "DX12Shader.h"
#include "../../Utility/InfoLog/InfoLog.h"
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
	desc.Format = ElementFormatToDXGIFormat(BackBufferFormat);
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
	SmartPTR<IDXGIAdapter4>& adapter,
	uint32_t width, uint32_t height, size_t node)
	: m_backend(backend), m_swapChain(swapChain), m_device(device), m_adapter(adapter),
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
		LOG_ERROR("Doesn't support this command unit type!");
		abort();
	}
}

bool DX12RenderDevice::QueryCommandExecutionState(size_t fenceValue, COMMAND_UNIT_TYPE type)
{
	switch (type) {
	case DEFAULT_COMMAND_UNIT:
		return m_commandUnitManager_graphics.IsFenceComplete(fenceValue);
	default:
		LOG_ERROR("Doesn't support this command unit type");
		abort();
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
		LOG_ERROR("doesn't support this command unit type");
		abort();
	}
	(*cmdUnit) = nullptr;
}

void DX12RenderDevice::UpdatePerFrame()
{
}

bool DX12RenderDevice::Present()
{
	if (FAILED(m_swapChain->Present(1, 0))) {
		LOG_WARN("Present Failed! Some error occur!");
	}
	/** 更新当前backbuffer的fence值，该fence值代表了当前backbuffer已经present完成
	 * 以备将来确认backbuffer是否可用 */
	m_backBufferFenceValues[m_curBackBufferIndex] = m_commandUnitManager_graphics.IncrementFence();

	uint64_t nextBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
	/** 等待下一个backbuffer可用 */
	m_commandUnitManager_graphics.WaitForFence(m_backBufferFenceValues[nextBackBuffer]);
	m_curBackBufferIndex = nextBackBuffer;
	++m_currentFrameCount;
	return true;
}

CommandUnit * DX12RenderDevice::RequestCommandUnit(COMMAND_UNIT_TYPE type)
{
	CommandUnit* cu = nullptr;
	switch (type) {
	case DEFAULT_COMMAND_UNIT:
		cu = m_commandUnitManager_graphics.AllocateCommandUnit();
		break;
	default:
		LOG_ERROR("Doesn't support this command unit type!");
		abort();
	}
	if (cu == nullptr) {
		LOG_ERROR("Request command unit failed!");
		abort();
	}
	return cu;
}

DX12RenderDevice::~DX12RenderDevice()
{
	/** 需要等待所有相关联的GPU指令执行完成后才能退出 */
	//m_commandListManager.IdleGPU();
	m_commandUnitManager_graphics.WaitForIdle();
	for (size_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx)
		m_depthStencilBuffers[idx].reset();
}

bool DX12RenderDevice::Initialize(std::string config)
{
	/** 初始化必须的组件 */
	if (m_resourceManager.Initialize(m_adapter.Get()) == false) {
		LOG_ERROR("Initialized Resource Manager failed!");
		return false;
	}
	if (m_rtvDescriptorHeap.Initialize(m_device.Get(), NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_RTV) == false) {
		LOG_ERROR("Initialized RTV Descriptor heap failed!");
		return false;
	}
	if (m_dsvDescriptorHeap.Initialize(m_device.Get(), NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_DSV) == false) {
		LOG_ERROR("Initialized DSV descriptor heap failed!");
		return false;
	}
	if (m_graphics_srv_cbv_uav_descriptorHeap.Initialize(m_device.Get(), NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) == false) {
		LOG_ERROR("Initialized SRV CBV UAV descriptor heap failed!");
		return false;
	}

	/** 构建RT */
	{
		std::lock_guard<std::mutex> lg(m_rtvPointerToHandle_mutex);
		for (uint8_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx) {
			XifFailed(m_swapChain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx])), {
				LOG_ERROR("Create Back buffer %d FAILED!", idx);
				return false;
				});
			auto handle = m_rtvDescriptorHeap.RequestBlock();
			m_swapChainResources[idx].Initialize(m_backBuffers[idx].Get(), m_device.Get(), handle);
			m_rtvPointerToHandle.insert({ &m_swapChainResources[idx], handle });
			m_backBuffers[idx]->SetName(L"BackBuffer");
		}
	}
	/** 构建DSV */
	{
		for (size_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx) {
			m_depthStencilBuffers[idx].reset(CreateTexture2D(ScreenWidth, ScreenHeight, 1, 1, ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT, ResourceStatus(
				RESOURCE_USAGE_DEPTH_STENCIL, RESOURCE_FLAG_STABLY)));
			if (m_depthStencilBuffers[idx] == nullptr) {
				LOG_ERROR("Create Depth Stencil Buffer %d Failed!", idx);
				return false;
			}
		}
	}

	RenderDevice::Initialize(config);

	auto cmdUnit = RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	for (size_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx) {
		cmdUnit->TransferState(m_depthStencilBuffers[idx].get(), RESOURCE_STATE_DEPTH_WRITE);
	}
	cmdUnit->Flush(true);
	FreeCommandUnit(&cmdUnit);
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
			if (ptr == nullptr) {
				LOG_ERROR("Resource Type and Resource doesn't match!");
				abort();
			}
			desc = ptr->GetRenderTargetView(0);
			m_device->CreateRenderTargetView(
				reinterpret_cast<ID3D12Resource*>(tex->GetResource()),
				&desc, allocateHandle);
		}
		else {
			LOG_ERROR("Doesn't support this texture type to be RT");
			abort();
		}
		m_rtvPointerToHandle.insert({ tex, allocateHandle });
		return allocateHandle;
	}
	else {
		return iter->second;
	}
}

std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> DX12RenderDevice::PickupDepthStencilTarget(GPUResource* tex) thread_safe {
	std::lock_guard<std::mutex> lg(m_dsvPointerToHandle_mutex);
	auto iter = m_dsvPointerToHandle.find(tex);
	if (iter == m_dsvPointerToHandle.end()) {
		auto allocateHandle = m_dsvDescriptorHeap.RequestBlock();
		D3D12_DEPTH_STENCIL_VIEW_DESC desc;
		if (tex->Type() == GPU_RESOURCE_TYPE_TEXTURE2D) {
			auto ptr = dynamic_cast<DX12Texture2D*>(tex);
			if (ptr == nullptr) {
				LOG_ERROR("Resource Type and Resource doesn't match!");
				abort();
			}
			desc = ptr->GetDepthStencilView();
			m_device->CreateDepthStencilView(
				reinterpret_cast<ID3D12Resource*>(tex->GetResource()),
				&desc, allocateHandle);
		}
		else {
			LOG_ERROR("Doesn't support this texture type to be DS");
			abort();
		}
		m_dsvPointerToHandle.insert({ tex, allocateHandle });
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
		LOG_ERROR("Doesn't support this command unit type");
		abort();
	}
	if (range.Valid() == false) {
		LOG_ERROR("Request descriptor block failed!");
		abort();
	}
	return range;
}

void DX12RenderDevice::ReleaseDescriptorBlocks(AllocateRange range, size_t fenceValue, COMMAND_UNIT_TYPE type)
{
	if (type == DEFAULT_COMMAND_UNIT) {
		m_graphics_srv_cbv_uav_descriptorHeap.ReleaseBlock(range, fenceValue);
	}
	else {
		LOG_ERROR("Doesn't support this command unit type");
		abort();
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
		LOG_WARN("Shader %s or %s haven't been initialized! You may forget to call RenderBackend::InitializedShaderObject", vs->Name(), ps->Name());
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

Texture2D* DX12RenderDevice::CreateTexture2D(size_t width, size_t height, size_t miplevels, size_t arrSize, ElementFormatType format, ResourceStatus status)
{
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	if (status.isFrequently() || status.isOnce()) heapType = D3D12_HEAP_TYPE_UPLOAD;
	else if (status.isReadBack()) heapType = D3D12_HEAP_TYPE_READBACK;
	auto [pRes, state] = m_resourceManager.RequestTexture(
		width, height, 0,
		ElementFormatToDXGIFormat(format),
		ResourceStatusToResourceFlag(status),
		arrSize, miplevels, false
	);
	if (pRes == nullptr) return nullptr;
	DX12Texture2D* newTex2D = new DX12Texture2D();
	newTex2D->m_width = width;
	newTex2D->m_height = height;
	newTex2D->m_mipLevels = miplevels;
	newTex2D->m_arrSize = arrSize;
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
	if (pDest->MemFootprint() < srcSize + destOffset) {
		LOG_WARN("Buffer size is not large enough!");
		return false;
	}
	if (pDest->m_status.isStably() == false) {
		uint8_t* pData = nullptr;
		pRes->Map(0, nullptr, (void**)&pData);
		pData += destOffset;
		memcpy(pData, pSrc, srcSize);
		pRes->Unmap(0, nullptr);
	}
	else {
		auto [pIntermediateRes, state] = m_resourceManager.RequestBuffer(srcSize, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
		if (pIntermediateRes == nullptr || state != D3D12_RESOURCE_STATE_GENERIC_READ) {
			LOG_ERROR("Request intermediate buffer failed!");
			abort();
		}
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

bool DX12RenderDevice::WriteToTexture2D(const std::vector<ImageDesc>& srcDesc, Texture2D* pDest, CommandUnit* cmdUnit)
{
	DX12CommandUnit* pUnit = dynamic_cast<DX12CommandUnit*>(cmdUnit);
	DX12Texture2D* pTex = dynamic_cast<DX12Texture2D*>(pDest);
	ID3D12Resource* pRes = reinterpret_cast<ID3D12Resource*>(pTex->GetResource());
	std::vector<D3D12_SUBRESOURCE_DATA> subresources(srcDesc.size());
	for (int i = 0; i < subresources.size(); ++i) {
		subresources[i].pData = srcDesc[i].data;
		subresources[i].RowPitch = srcDesc[i].rowPitch;
		subresources[i].SlicePitch = srcDesc[i].slicePitch;
	}
	if (subresources.size() > pDest->MipLevels()) { /**< 即将写入的子资源数量必须能够被pDest容纳 */
		LOG_ERROR("Number of subresources is out of range!");
		return false;
	}
	size_t intermediateBufferSize = GetRequiredIntermediateSize(pRes, 0, subresources.size());
	auto [pIntermediateRes, state] = m_resourceManager.RequestBuffer(intermediateBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
	if (pIntermediateRes == nullptr || state != D3D12_RESOURCE_STATE_GENERIC_READ) {
		LOG_ERROR("Request intermediate buffer failed!");
		return false;
	}
	if (UpdateSubresources(pUnit->m_graphicsCmdList.Get(), pRes, pIntermediateRes, 0, 0, subresources.size(), subresources.data())
		== 0) {
		LOG_ERROR("Update subresource failed!");
		abort();
	}
	cmdUnit->Flush(true);
	m_resourceManager.ReleaseResource(pIntermediateRes);
	return true;
}

bool DX12RenderDevice::WriteToTexture2DArr(const std::vector<std::vector<ImageDesc>>& srcDesc, Texture2D* pDest, CommandUnit* cmdUnit)
{
	DX12CommandUnit* pUnit = dynamic_cast<DX12CommandUnit*>(cmdUnit);
	DX12Texture2D* pTex = dynamic_cast<DX12Texture2D*>(pDest);
	ID3D12Resource* pRes = reinterpret_cast<ID3D12Resource*>(pTex->GetResource());
#ifdef _DEBUG
	// 检查srcDesc中每个子数组长度是否相同
	for (int i = 0; i < srcDesc.size() - 1; ++i) {
		if (srcDesc[i].size() != srcDesc[i + 1].size()) {
			LOG_ERROR("Invalid Source Data!, srcDesc[%d].size() != srcDesc[%d].size()", i, i + 1);
			abort();
		}
	}
#endif
	const size_t numOfSubresources = srcDesc.size() * srcDesc[0].size();
	if (numOfSubresources > pDest->ArrSize() * pDest->MipLevels()) {
		LOG_ERROR("Request intermediate buffer failed! Number of provieded subresources: %d, Number of actual needed subresources: %d",
			numOfSubresources, pDest->ArrSize() * pDest->MipLevels());
		return false;
	}
	std::vector<D3D12_SUBRESOURCE_DATA> subresources(numOfSubresources);
	{
		size_t i = 0;
		for (auto& srcArr : srcDesc) {
			for (auto& srcDescPtr : srcArr) {
				subresources[i].pData = srcDescPtr.data;
				subresources[i].RowPitch = srcDescPtr.rowPitch;
				subresources[i].SlicePitch = srcDescPtr.slicePitch;
				++i;
			}
		}
	}
	size_t intermediateBufferSize = GetRequiredIntermediateSize(pRes, 0, subresources.size());
	auto [pIntermediateRes, state] = m_resourceManager.RequestBuffer(intermediateBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
	if (pIntermediateRes == nullptr || state != D3D12_RESOURCE_STATE_GENERIC_READ) {
		LOG_ERROR("Request intermediate buffer failed!");
		return false;
	}
	if (UpdateSubresources(pUnit->m_graphicsCmdList.Get(), pRes, pIntermediateRes, 0, 0, subresources.size(), subresources.data())
		== 0) {
		LOG_ERROR("Update subresource failed!");
		abort();
	}
	cmdUnit->Flush(true);
	m_resourceManager.ReleaseResource(pIntermediateRes);
	return true;
}

bool DX12RenderDevice::ReadFromTexture2D(std::vector<uint8_t>& output, Texture2D* pSrc, CommandUnit* cmdUnit)
{
	DX12Texture2D* pTex = dynamic_cast<DX12Texture2D*>(pSrc);
	DX12CommandUnit* pUnit = dynamic_cast<DX12CommandUnit*>(cmdUnit);
	ID3D12Resource* pRes = reinterpret_cast<ID3D12Resource*>(pTex->GetResource());
	auto&& desc = pRes->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT numRows = 0;
	UINT64 rowSizeInBytes = 0;
	UINT64 requireSize = 0;
	m_device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &requireSize);
	auto [intermediateBufferPtr, state] = m_resourceManager.RequestBuffer(requireSize, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
		D3D12_HEAP_TYPE_READBACK);
	if (intermediateBufferPtr == nullptr) {
		LOG_ERROR("Create Intermediate buffer failed!");
		return false;
	}
	cmdUnit->TransferState(pSrc, RESOURCE_STATE_COPY_SRC);
	CD3DX12_TEXTURE_COPY_LOCATION src_location(pRes, 0);
	CD3DX12_TEXTURE_COPY_LOCATION dest_location(intermediateBufferPtr, footprint);
	pUnit->m_graphicsCmdList->CopyTextureRegion(&dest_location, 0, 0, 0, &src_location, nullptr);
	cmdUnit->Flush(true);
	void* pData = nullptr;
	if (FAILED(intermediateBufferPtr->Map(0, nullptr, &pData))) {
		LOG_ERROR("Map Intermediate buffer failed!");
		m_resourceManager.ReleaseResource(intermediateBufferPtr);
		return false;
	}
	/** 返回的数据不包含padding! */
	size_t bytePerRow = footprint.Footprint.Width * CalculateBPPFromDXGI_FORMAT(desc.Format);
	size_t rowPadding = footprint.Footprint.RowPitch - bytePerRow;
	output = std::vector<uint8_t>(bytePerRow * desc.Height);
	uint8_t* srcData = reinterpret_cast<uint8_t*>(pData);
	uint8_t* destData = output.data();
	for (size_t row = 0; row < desc.Height; ++row) {
		memcpy(destData, srcData, bytePerRow);
		destData += bytePerRow;
		srcData += footprint.Footprint.RowPitch;
	}
	intermediateBufferPtr->Unmap(0, nullptr);
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
