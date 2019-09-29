#include "DX12RenderDevice.h"
#include <cassert>
#include <optional>

#define XifFailed(function, behavior) if (FAILED(function)) behavior

BEG_NAME_SPACE

UINT GDescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

DX12RenderDevice::DX12RenderDevice(DX12RenderBackend& backend,
	SmartPTR<ID3D12CommandQueue>& queue,
	SmartPTR<IDXGISwapChain3>& swapChain,
	SmartPTR<ID3D12Device>& device,
	uint32_t width, uint32_t height)
	: m_backend(backend), m_swapChain(swapChain), m_device(device),
	m_backBuffers(decltype(m_backBuffers)(NUMBER_OF_BACK_BUFFERS)), m_curBackBufferIndex(0), RenderDevice(width, height),
	m_resourceManager(DX12ResourceManager(device.Get())), m_pipelineManager(device.Get()) {
	for (uint8_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		GDescriptorHandleIncrementSize[i] = m_device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	/** 初始化指令执行单位 */
	for (size_t commandUnitType = 0; commandUnitType < NUMBER_OF_COMMAND_UNIT_TYPE; ++commandUnitType) {
		m_commandUnits[commandUnitType] = DX12CommandUnit(this);
		if (commandUnitType == DEFAULT_COMMAND_UNIT) m_commandUnits[commandUnitType].Setup(queue);
		else m_commandUnits[commandUnitType].Setup(
			CommandUnitTypeToDX12CommandListType(
				static_cast<COMMAND_UNIT_TYPE>(commandUnitType)));
	}
}

bool DX12RenderDevice::Initialize(std::string config)
{
	/** 构建特殊资源 */
	for (uint8_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx) {
		XifFailed(m_swapChain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx])), {
			FLOG("%s: Create Back buffer %d FAILED!\n", __FUNCTION__, idx);
			return false;
		});
	}
	/** 构建backbuffers */
	for (size_t index = 0; index < m_backBuffers.size(); ++index) {
		
	}

#ifdef _DEBUG
	/** 检查所有的特殊资源都已经构建完成 */
	for (uint8_t i = 0; i < NUMBER_OF_SPECIAL_BUFFER_RESOURCE; ++i) {
		auto res = m_buffers.find(BufferHandle(i));
		assert(res != m_buffers.end() && res->second.ptr != nullptr);
	}
	for (uint8_t i = 0; i < NUMBER_OF_SPECIAL_TEXTURE_RESOURCE; ++i) {
		auto res = m_textures.find(TextureHandle(i));
		assert(res != m_textures.end() && res->second.ptr != nullptr);
	}
#endif // _DEBUG
	/** 构建Descriptor heaps */
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	desc.NumDescriptors = NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP;
	if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)))) {
		FLOG("%s: Create RTV heap failed\n", __FUNCTION__);
		return false;
	}
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	desc.NumDescriptors = NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP;
	if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)))) {
		FLOG("%s: Create RTV heap failed\n", __FUNCTION__);
		return false;
	}
	/** 构造默认viewport和裁剪矩形 */
	/** TODO: 提供用户定义viewport设置以及裁剪矩阵设置 */
	m_viewport.Width = ScreenWidth; m_viewport.Height = ScreenHeight;
	m_viewport.MaxDepth = 1.0f; m_viewport.MinDepth = 0.0f;
	m_viewport.TopLeftX = m_viewport.TopLeftY = 0;
	m_scissorRect.top = m_scissorRect.left = 0;
	m_scissorRect.right = ScreenWidth; m_scissorRect.bottom = ScreenHeight;
	/** 初始化必须的组件 */
	RenderDevice::Initialize(config);
	return true;
}


std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> DX12RenderDevice::PickupRenderTarget(TextureHandle tex) thread_safe {
	TextureInfo* texInfoPtr = PickupTexture(tex);
	if (texInfoPtr == nullptr) {
		FLOG("%s: Invalid texture handle value and can't pick up any render target!\n", __FUNCTION__);
		return {}; /**< 返回一个空的值 */
	}
	uint64_t& code = texInfoPtr->renderTargetViewCode;
	{
		std::lock_guard<decltype(m_rtvHeapGenLock)> lg(m_rtvHeapGenLock);
		if (TextureInfo::DecodeRTVCodeIndex(code) == NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP ||
			m_rtvHeapGen[TextureInfo::DecodeRTVCodeIndex(code)] != TextureInfo::DecodeRTVCodeGen(code)) {
			/** 当前RTV不可用，需要重新创建 */
			uint32_t next = m_rtvHeapGen.back()++;
			if (next >= NUMBER_OF_DESCRIPTOR_IN_RTV_HEAP) {
				/** 当前RTV已满，需要回到开头位置 */
				m_rtvHeapGen.back() = 1;
				next = 0;
			}
			++m_rtvHeapGen[next];
			TextureInfo::EncodeRTVCodeIndex(code, next);
			TextureInfo::EncodeRTVCodeGen(code, m_rtvHeapGen[next]);

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			D3D12_RESOURCE_DESC resDesc;
			resDesc = texInfoPtr->ptr->GetDesc();
			rtvDesc.Format = resDesc.Format;
			/** TODO: 目前只支持tex2d的0号Mipmap作为rendertarget */
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			m_device->CreateRenderTargetView(texInfoPtr->ptr, &rtvDesc,
				{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr /**< 计算新的descriptor heap的位置 base + next * increment */
				+ next * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] });
		}
	}
	return { { m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr
		+ TextureInfo::DecodeRTVCodeIndex(code) * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] } };
}

std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> DX12RenderDevice::PickupDepthStencilTarget(TextureHandle tex) thread_safe {
	TextureInfo* texInfoPtr = PickupTexture(tex);
	if (texInfoPtr == nullptr) {
		FLOG("%s: Invalid texture handle value and can't pick up any depth stencil!\n", __FUNCTION__);
		return {}; /**< 返回一个空的值 */
	}
	uint64_t& code = texInfoPtr->depthStencilViewCode;
	{
		std::lock_guard<decltype(m_dsvHeapGenLock)> lg(m_dsvHeapGenLock);
		if (TextureInfo::DecodeDSVCodeIndex(code) == NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP ||
			m_dsvHeapGen[TextureInfo::DecodeDSVCodeIndex(code)] != TextureInfo::DecodeDSVCodeGen(code)) {
			/** 当前DSV不可用，需要重建 */
			uint32_t next = m_dsvHeapGen.back()++;
			if (next >= NUMBER_OF_DESCRIPTOR_IN_DSV_HEAP) {
				m_dsvHeapGen.back() = 1;
				next = 0;
			}
			++m_dsvHeapGen[next];
			TextureInfo::EncodeDSVCodeGen(code, m_dsvHeapGen[next]);
			TextureInfo::EncodeDSVCodeIndex(code, next);

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			D3D12_RESOURCE_DESC resDesc;
			resDesc = texInfoPtr->ptr->GetDesc();
			dsvDesc.Format = resDesc.Format;
			/** TODO: 目前仅支持DSV均可读写 */
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			/** TODO: 目前仅支持DSV为tex2d，且只有0号mipmap生效 */
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			m_device->CreateDepthStencilView(texInfoPtr->ptr, &dsvDesc,
				{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr
				+ next * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] });
		}
	}
	return {
	{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr
		+ TextureInfo::DecodeDSVCodeIndex(code) * m_descriptorHeapIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]  }
	};
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
	DX12GraphicsPipelineObject graphicsPSO(vs, ps, rastOpt, outputOpt, vtxAttribList);
	auto dx12vs = GShaderManager[vs->GetHandle()];
	auto dx12ps = GShaderManager[ps->GetHandle()];
	if (m_pipelineManager.Build(graphicsPSO, dx12vs, dx12ps) == false) return nullptr;
}

END_NAME_SPACE
