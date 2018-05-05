#include "Pass.h"
#include "Resource.h"
#include "InfoLog.h"
#include "Shader.h"
#include "Mesh.h"
#include "RasterState.h"

BindingData<RasterState> ShadingPass::Def_RasterState;
D3D11_VIEWPORT ShadingPass::Def_ViewPort;

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Clear Scheme   ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
ClearScheme::ClearScheme(bool bc, bool ac) : bc(bc), ac(ac) {}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   BasePass   ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

BasePass& BasePass::BindSource(IConstantBuffer * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData<IConstantBuffer> bd;
	bd.bindTarget = sbt;
	bd.resPointer = res;
	bd.slot = slot;
	//m_bdOfConstBuffer.push_back(bd);
	UINT key = UINT(sbt) * 100 + UINT(slot);
	m_bdOfConstBuffer[key] = bd;
	return *this;
}

BasePass& BasePass::BindSource(ITexture * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData<ITexture> bd;
	bd.bindTarget = sbt;
	bd.resPointer = res;
	bd.slot = slot;
	m_bdOfTexture.push_back(bd);
	return *this;
}

BasePass& BasePass::BindSource(IBuffer * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData<IBuffer> bd;
	bd.bindTarget = sbt;
	bd.resPointer = res;
	bd.slot = slot;
	m_bdOfBuffer.push_back(bd);
	return *this;
}

BasePass& BasePass::BindSource(ISamplerState * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData<ISamplerState> bd;
	bd.bindTarget = sbt;
	bd.resPointer = res;
	bd.slot = slot;
	m_bdOfSamplerState.push_back(bd);
	return *this;
}

BasePass& BasePass::BindSource(IUnorderAccess* res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData<IUnorderAccess> bd;
	bd.bindTarget = sbt;
	bd.resPointer = res;
	bd.slot = slot;
	m_bdOfUnorderAccess.push_back(bd);
	return *this;
}

BasePass& BasePass::Run(ID3D11DeviceContext* devCtx) {
	// binding resources
	ITERATE_BINDING_DATA_VECTOR(devCtx, m_bdOfBuffer);
	ITERATE_BINDING_DATA_MAP(devCtx, m_bdOfConstBuffer);
	ITERATE_BINDING_DATA_VECTOR(devCtx, m_bdOfSamplerState);
	ITERATE_BINDING_DATA_VECTOR(devCtx, m_bdOfTexture);
	ITERATE_BINDING_DATA_VECTOR(devCtx, m_bdOfUnorderAccess);
	ITERATE_BINDING_DATA_VECTOR(devCtx, m_bdOfUnknownObject);

	return *this;
}

BasePass& BasePass::End(ID3D11DeviceContext* devCtx) {
	// Unbind resource
	ITERATE_UNBINDING_DATA_VECTOR(devCtx, m_bdOfBuffer);
	ITERATE_UNBINDING_DATA_MAP(devCtx, m_bdOfConstBuffer);
	ITERATE_UNBINDING_DATA_VECTOR(devCtx, m_bdOfSamplerState);
	ITERATE_UNBINDING_DATA_VECTOR(devCtx, m_bdOfTexture);
	ITERATE_UNBINDING_DATA_VECTOR(devCtx, m_bdOfUnorderAccess);

	ITERATE_UNBINDING_DATA_VECTOR(devCtx, m_bdOfUnknownObject);

	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Shading Pass   ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

ShadingPass::ShadingPass(VertexShader* vs, PixelShader* ps, GeometryShader* gs)
	:m_vs(vs), m_ps(ps), m_gs(gs),
	m_ds(nullptr), m_viewport(nullptr), m_specialDrawCall(0){}

ShadingPass& ShadingPass::BindSource(IRenderTarget * rt, bool beforeClear, bool afterClear)
{
	m_rtvs.push_back(rt->GetRTV());
	m_rtvsClearSchemes.push_back(ClearScheme(beforeClear, afterClear));
	return *this;
}

ShadingPass& ShadingPass::BindSource(IDepthStencil* ds, bool beforeClear, bool afterClear) {
	m_ds = ds;
	m_dsClearScheme.bc = beforeClear;
	m_dsClearScheme.ac = afterClear;
	return *this;
}

ShadingPass& ShadingPass::BindSource(Mesh * res, ShaderBindTarget sbt, SIZE_T slot)
{
	m_meshBindingData.resPointer = res;
	m_meshBindingData.bindTarget = sbt;
	m_meshBindingData.slot = slot;
	return *this;
}

ShadingPass& ShadingPass::BindSource(RasterState * rs, D3D11_VIEWPORT* vp, UINT numOfvp)
{
	if (rs) {
		m_rasterStateData.resPointer = rs;
	}
	if (vp) {
		m_viewport = vp;
		m_numOfViewport = numOfvp;
	}
	return *this;
}

ShadingPass & ShadingPass::BindSource(BlendState * bs)
{
	m_blendStateData.resPointer = bs;
	return *this;
}

BasePass& BasePass::BindSource(UnknownObject* res, ShaderBindTarget sbt, SIZE_T slot) {
	BindingData<UnknownObject> bd;
	bd.resPointer = res;
	bd.bindTarget = sbt;
	bd.slot = slot;
	m_bdOfUnknownObject.push_back(bd);
	return *this;
}

ShadingPass& ShadingPass::Run(ID3D11DeviceContext* devCtx) {
	if (m_vs == nullptr) {
		MLOG(LW, __FUNCTION__, LL, "expect vertex/pixel shader!");
		return *this;
	}
	if (m_meshBindingData.resPointer == nullptr && !m_specialDrawCall) {
		MLOG(LW, __FUNCTION__, LL, "There are no draw call!");
		return *this;
	}
	static float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_vs->Bind(devCtx);
	if (m_ps) m_ps->Bind(devCtx);
	if (m_gs) m_gs->Bind(devCtx);

	BasePass::Run(devCtx);

	// clear RT
	for (int i = 0; i < m_rtvs.size(); ++i) {
		if (m_rtvsClearSchemes[i].bc)
			devCtx->ClearRenderTargetView(m_rtvs[i], clearColor);
	}
	// clear DS
	if (m_dsClearScheme.bc && m_ds) devCtx->ClearDepthStencilView(m_ds->GetDSV(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Set Raster state
	if (m_rasterStateData.resPointer != nullptr) {
		m_rasterStateData.resPointer->Bind(devCtx, SBT_UNKNOWN, 0);
	}
	else {
		// use default raster state
		Def_RasterState.resPointer->Bind(devCtx, SBT_UNKNOWN, 0);
	}

	// Set View Port
	if (m_viewport) {
		devCtx->RSSetViewports(m_numOfViewport, m_viewport);
	}
	else {
		devCtx->RSSetViewports(1, &Def_ViewPort);
	}

	// binding RT
	if (m_ds) {
		if (m_rtvs.size()) devCtx->OMSetRenderTargets(m_rtvs.size(), &m_rtvs[0], m_ds->GetDSV());
		// 适用于只设置vs的情况
		else devCtx->OMSetRenderTargets(0, nullptr, m_ds->GetDSV());
	}
	else if (m_rtvs.size()) devCtx->OMSetRenderTargets(m_rtvs.size(), &m_rtvs[0], nullptr);
	else devCtx->OMSetRenderTargets(0, nullptr, nullptr);

	// bind unorder access view in pixel shader
	std::vector<ID3D11UnorderedAccessView*> uavs;
	std::vector<UINT> initialCount;
	UINT startSlot = -1;
	for (auto& iter : m_bdOfUnorderAccess) {
		if (iter.bindTarget == SBT_PIXEL_SHADER) {
			uavs.push_back(iter.resPointer->GetUAV());
			if (startSlot == -1) startSlot = iter.slot;
		}
		initialCount.push_back(0);
	}

	if (uavs.size()) {
		devCtx->OMSetRenderTargetsAndUnorderedAccessViews(
			D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
			startSlot, uavs.size(), &uavs[0], &initialCount[0]
		);
	}

	// Set blend state
	if (m_blendStateData.resPointer) {
		m_blendStateData.resPointer->Bind(devCtx);
	}

	// bind mesh and submit draw call
	if (m_specialDrawCall) devCtx->Draw(m_specialDrawCall, 0);
	else m_meshBindingData.resPointer->Bind(devCtx, m_meshBindingData.bindTarget, m_meshBindingData.slot);

	return *this;
}

ShadingPass& ShadingPass::End(ID3D11DeviceContext* devCtx) {
	if (m_vs == nullptr || m_ps == nullptr) {
		return *this;
	}
	if (m_meshBindingData.resPointer == nullptr && !m_specialDrawCall) {
		return *this;
	}
	static float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_vs->Unbind(devCtx);
	m_ps->Unbind(devCtx);
	if (m_gs) m_gs->Unbind(devCtx);

	BasePass::End(devCtx);

	// clear RT
	for (int i = 0; i < m_rtvs.size(); ++i) {
		if (m_rtvsClearSchemes[i].ac)
			devCtx->ClearRenderTargetView(m_rtvs[i], clearColor);
	}
	// clear DS
	if (m_dsClearScheme.ac && m_ds) devCtx->ClearDepthStencilView(m_ds->GetDSV(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Unbind raster state
	if (m_rasterStateData.resPointer != nullptr) {
		m_rasterStateData.resPointer->Unbind(devCtx, SBT_UNKNOWN, 0);
	}
	else {
		Def_RasterState.resPointer->Unbind(devCtx, SBT_UNKNOWN, 0);
	}

	// Unbind RT
	//devCtx->OMSetRenderTargets(0, NULL, nullptr);
	ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
	UINT nullInitValue[1] = { 0 };
	devCtx->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, nullUAV, nullInitValue);

	// Unbind Blend State
	if (m_blendStateData.resPointer) {
		m_blendStateData.resPointer->Unbind(devCtx);
	}

	// unbind mesh
	if (!m_specialDrawCall) m_meshBindingData.resPointer->Unbind(devCtx, m_meshBindingData.bindTarget, m_meshBindingData.slot);

	return *this;
}

ShadingPass & ShadingPass::SpecialDrawCall(UINT numOfEle)
{
	m_specialDrawCall = numOfEle;
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Computing Pass   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

ComputingPass::ComputingPass(ComputeShader * cs, DirectX::XMFLOAT3 groups)
	: m_cs(cs), m_groups(groups) {}

ComputingPass& ComputingPass::Run(ID3D11DeviceContext* devCtx) {
	if (!m_cs) {
		MLOG(LW, __FUNCTION__, LL, "no compute shader!");
		return *this;
	}
	m_cs->Bind(devCtx);

	BasePass::Run(devCtx);

	devCtx->Dispatch(m_groups.x, m_groups.y, m_groups.z);

	return *this;
}

ComputingPass& ComputingPass::End(ID3D11DeviceContext* devCtx) {
	if (!m_cs) return *this;
	m_cs->Unbind(devCtx);
	BasePass::End(devCtx);
	return *this;
}
