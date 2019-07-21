#include "DX12RenderBasic.h"

BEG_NAME_SPACE
BufMgr::IndexType BufMgr::RequestBufferHandle() {
	std::lock_guard<std::mutex> lg(bm_mutex);
	BufferHandle res;
	if (freeIndices.empty()) {
		res = BufferHandle(buffers.size());
		buffers.push_back(SmartPTR<ID3D12Resource>());
	}
	else {
		res = freeIndices.back();
		freeIndices.pop_back();
	}
	return res;
}

void BufMgr::RevertBufferHandle(IndexType index) {
	std::lock_guard<std::mutex> lg(bm_mutex);
	freeIndices.push_back(index);
}

void BufMgr::SetBuffer(ElementType && newElement, IndexType index) {
	std::lock_guard<std::mutex> lg(bm_mutex);
	buffers[index.value()] = newElement;
}

const BufMgr::ElementType& BufMgr::operator[](const IndexType& index) const {
	std::lock_guard<std::mutex> lg(bm_mutex);
	return buffers[index.value()];
}

BufMgr::ElementType& BufMgr::operator[](const IndexType& index) {
	std::lock_guard<std::mutex> lg(bm_mutex);
	return buffers[index.value()];
}

ID3D12GraphicsCommandList* CommandListMgr::RequestCmdList() {
	std::lock_guard<std::mutex> lg(clm_mutex);
	ElementType ele;
	SmartPTR<ID3D12Device> dev;
	if (freeCmdLists.empty()) {
		allocator->GetDevice(IID_PPV_ARGS(&dev));
		dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&ele));
		cmdLists.push_back(ele);
	}
	else {
		ele = cmdLists[freeCmdLists.back()];
		freeCmdLists.pop_back();
		ele->Reset(allocator.Get(), nullptr);
	}
	return ele.Get();
}


void TransientResourceMgr::Store(ElementType& e) thread_safe {
	std::lock_guard<std::mutex> lg(trm_mutex);
	resources.push_back(e);
}

QueueProxy::QueueProxy(QueueProxy && qp) {
	queue.Swap(qp.queue);
	fence.Swap(qp.fence);
	fenceValue = qp.fenceValue;
	fenceEvent = qp.fenceEvent;
	qp.fenceEvent = NULL;
}

QueueProxy& QueueProxy::operator=(QueueProxy&& rhs) {
	queue.Swap(rhs.queue);
	fence.Swap(rhs.fence);
	fenceValue = rhs.fenceValue;
	fenceEvent = rhs.fenceEvent;
	rhs.fenceEvent = NULL;
	return *this;
}

void QueueProxy::Execute(uint32_t numCommandLists, ID3D12CommandList * const * ppCommandLists)
{
	queue->ExecuteCommandLists(numCommandLists, ppCommandLists);
	queue->Signal(fence.Get(), ++fenceValue);
	fence->SetEventOnCompletion(fenceValue, fenceEvent);
	WaitForSingleObject(fenceEvent, INFINITE);
}



D3D12_BLEND_DESC AnalyseBlendingOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
{
	/** TODO: 完善对blend的支持，目前仅提供默认(无blend)操作 */
	D3D12_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		desc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	return desc;
}

D3D12_DEPTH_STENCIL_DESC AnalyseDepthStencilOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
{
	/** TODO: 完善深度模板操作的支持，目前仅支持默认的深度测试，不支持模板测试 */
	D3D12_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = osOpt.enableDepthTest;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.StencilEnable = FALSE;
	desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	desc.FrontFace = defaultStencilOp;
	desc.BackFace = defaultStencilOp;
	return desc;
}

D3D12_RASTERIZER_DESC AnalyseRasterizerOptionsFromRasterizeOptions(const RasterizeOptions & rastOpt) thread_safe
{
	D3D12_RASTERIZER_DESC desc;
	desc.FillMode = FillModeToDX12FillMode(rastOpt.fillMode);
	desc.CullMode = CullModeToCullMode(rastOpt.cullMode);
	desc.FrontCounterClockwise = rastOpt.counterClockWiseIsFront;
	/** TODO: 支持以下光栅化设置 */
	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return desc;
}

D3D12_STATIC_SAMPLER_DESC AnalyseStaticSamplerFromSamplerDescriptor(const SamplerDescriptor & desc, uint8_t spaceIndex, uint8_t registerIndex) thread_safe
{
	D3D12_STATIC_SAMPLER_DESC samplerDesc;
	samplerDesc.RegisterSpace = spaceIndex;
	samplerDesc.ShaderRegister = registerIndex;
	samplerDesc.Filter = FilterTypeToDX12FilterType(desc.filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(desc.uAddrMode);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(desc.vAddrMode);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(desc.wAddrMode);
	/** 静态sampler不能提供可设置的边界颜色只能使用默认值黑/白 */
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	return samplerDesc;
}

D3D12_SAMPLER_DESC AnalyseSamplerFromSamperDescriptor(const SamplerDescriptor& desc) thread_safe {
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = FilterTypeToDX12FilterType(desc.filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(desc.uAddrMode);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(desc.vAddrMode);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(desc.wAddrMode);
	samplerDesc.BorderColor[0] = desc.borderColor[0];
	samplerDesc.BorderColor[1] = desc.borderColor[1];
	samplerDesc.BorderColor[2] = desc.borderColor[2];
	samplerDesc.BorderColor[3] = desc.borderColor[3];
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	return samplerDesc;
}

END_NAME_SPACE
