#include "Pass.h"
#include "Resource.h"
#include "InfoLog.h"
#include "Shader.h"
#include "Mesh.h"
#include "RasterState.h"

BindingData ShadingPass::Def_RasterState;
D3D11_VIEWPORT ShadingPass::Def_ViewPort;

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Clear Scheme   ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
ClearScheme::ClearScheme(bool bc, bool ac) : bc(bc), ac(ac) {}

BindingData::BindingData(SourceType st,
	ShaderBindTarget sbt, SIZE_T slot)
	: resType(st), bindTarget(sbt), slot(slot) {
	resPointer.object = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   BasePass   ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void BasePass::BindSource(IConstantBuffer * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData bd;
	bd.bindTarget = sbt;
	bd.resPointer.constBuf = res;
	bd.resType = MST_CONSTANT_BUFFER;
	bd.slot = slot;
	m_bindingData.push_back(bd);
}

void BasePass::BindSource(ITexture * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData bd;
	bd.bindTarget = sbt;
	bd.resPointer.tex = res;
	bd.resType = MST_TEXTURE;
	bd.slot = slot;
	m_bindingData.push_back(bd);
}

void BasePass::BindSource(IBuffer * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData bd;
	bd.bindTarget = sbt;
	bd.resPointer.buf = res;
	bd.resType = MST_BUFFER;
	bd.slot = slot;
	m_bindingData.push_back(bd);
}

void BasePass::BindSource(ISamplerState * res, ShaderBindTarget sbt, SIZE_T slot)
{
	BindingData bd;
	bd.bindTarget = sbt;
	bd.resPointer.sampler = res;
	bd.resType = MST_SAMPLER_STATE;
	bd.slot = slot;
	m_bindingData.push_back(bd);
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Shading Pass   ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

ShadingPass::ShadingPass(VertexShader* vs, PixelShader* ps, GeometryShader* gs)
	:m_vs(vs), m_ps(ps), m_gs(gs),
	m_ds(nullptr), m_viewport(nullptr){}

void ShadingPass::BindSource(IRenderTarget * rt, bool beforeClear, bool afterClear)
{
	m_rtvs.push_back(rt->GetRTV());
	m_rtvsClearSchemes.push_back(ClearScheme(beforeClear, afterClear));
}

void ShadingPass::BindSource(IDepthStencil* ds, bool beforeClear, bool afterClear) {
	m_ds = ds;
	m_dsClearScheme.bc = beforeClear;
	m_dsClearScheme.ac = afterClear;
}

void ShadingPass::BindSource(Mesh * res, ShaderBindTarget sbt, SIZE_T slot)
{
	m_meshBindingData.resPointer.object = res;
	m_meshBindingData.resType = MST_UNKNOWN_OBJECT;
	m_meshBindingData.bindTarget = sbt;
	m_meshBindingData.slot = slot;
}

void ShadingPass::BindSource(RasterState * rs, D3D11_VIEWPORT* vp)
{
	if (rs) {
		m_rasterStateData.resPointer.object = rs;
		m_rasterStateData.resType = MST_UNKNOWN_OBJECT;
	}
	if (vp) {
		m_viewport = vp;
	}
}

void ShadingPass::BindSource(UnknownObject* res, ShaderBindTarget sbt, SIZE_T slot) {
	if (sbt == SBT_GEOMETRY_SHADER && m_gs == nullptr) {
		MLOG(LW, __FUNCTION__, LL, "no geometry shader can not finish binding!");
		return;
	}
	BindingData bd;
	bd.resPointer.object = res;
	bd.resType = MST_UNKNOWN_OBJECT;
	bd.bindTarget = sbt;
	bd.slot = slot;
	m_bindingData.push_back(bd);
}

void ShadingPass::Run(ID3D11DeviceContext* devCtx) {
	if (m_vs == nullptr) {
		MLOG(LW, __FUNCTION__, LL, "expect vertex/pixel shader!");
		return;
	}
	if (m_meshBindingData.resType != MST_UNKNOWN_OBJECT) {
		MLOG(LW, __FUNCTION__, LL, "No mesh binding!");
		return;
	}
	static float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_vs->Bind(devCtx);
	if (m_ps) m_ps->Bind(devCtx);
	if (m_gs) m_gs->Bind(devCtx);
	// bind resource
	for (auto& iter : m_bindingData) {
		switch (iter.resType)
		{
		case MST_CONSTANT_BUFFER:
			iter.resPointer.constBuf->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_BUFFER:
			iter.resPointer.buf->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_TEXTURE:
			iter.resPointer.tex->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_SAMPLER_STATE:
			iter.resPointer.sampler->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_UNKNOWN_OBJECT:
			iter.resPointer.object->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		default:
			MLOG(LW, __FUNCTION__, LL, "Unknown binding target!");
			break;
		}
	}
	// clear RT
	for (int i = 0; i < m_rtvs.size(); ++i) {
		if (m_rtvsClearSchemes[i].bc)
			devCtx->ClearRenderTargetView(m_rtvs[i], clearColor);
	}
	// clear DS
	if (m_dsClearScheme.bc && m_ds) devCtx->ClearDepthStencilView(m_ds->GetDSV(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Set Raster state
	if (m_rasterStateData.resType == MST_UNKNOWN_OBJECT) {
		m_rasterStateData.resPointer.object->Bind(devCtx, SBT_UNKNOWN, 0);
	}
	else {
		// use default raster state
		Def_RasterState.resPointer.object->Bind(devCtx, SBT_UNKNOWN, 0);
	}

	// Set View Port
	if (m_viewport) {
		devCtx->RSSetViewports(1, m_viewport);
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

	// bind mesh and submit draw call
	m_meshBindingData.resPointer.object->Bind(devCtx, m_meshBindingData.bindTarget, m_meshBindingData.slot);
}

void ShadingPass::End(ID3D11DeviceContext* devCtx) {
	if (m_vs == nullptr || m_ps == nullptr) {
		return;
	}
	if (m_meshBindingData.resType != MST_UNKNOWN_OBJECT) {
		return;
	}
	static float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_vs->Unbind(devCtx);
	m_ps->Unbind(devCtx);
	if (m_gs) m_gs->Unbind(devCtx);
	// Unbind resource
	for (auto& iter : m_bindingData) {
		switch (iter.resType)
		{
		case MST_CONSTANT_BUFFER:
			iter.resPointer.constBuf->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_BUFFER:
			iter.resPointer.buf->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_TEXTURE:
			iter.resPointer.tex->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_SAMPLER_STATE:
			iter.resPointer.sampler->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_UNKNOWN_OBJECT:
			iter.resPointer.object->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		default:
			MLOG(LW, __FUNCTION__, LL, "Unknown binding target!");
			break;
		}
	}
	// clear RT
	for (int i = 0; i < m_rtvs.size(); ++i) {
		if (m_rtvsClearSchemes[i].ac)
			devCtx->ClearRenderTargetView(m_rtvs[i], clearColor);
	}
	// clear DS
	if (m_dsClearScheme.ac && m_ds) devCtx->ClearDepthStencilView(m_ds->GetDSV(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Unbind raster state
	if (m_rasterStateData.resType == MST_UNKNOWN_OBJECT) {
		m_rasterStateData.resPointer.object->Bind(devCtx, SBT_UNKNOWN, 0);
	}
	else {
		Def_RasterState.resPointer.object->Bind(devCtx, SBT_UNKNOWN, 0);
	}

	// Unbind RT
	devCtx->OMSetRenderTargets(0, NULL, nullptr);

	// unbind mesh and submit draw call
	m_meshBindingData.resPointer.object->Unbind(devCtx, m_meshBindingData.bindTarget, m_meshBindingData.slot);
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Computing Pass   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

ComputingPass::ComputingPass(ComputeShader * cs, DirectX::XMFLOAT3 groups)
	: m_cs(cs), m_groups(groups) {}

void ComputingPass::Run(ID3D11DeviceContext* devCtx) {
	if (!m_cs) {
		MLOG(LW, __FUNCTION__, LL, "no compute shader!");
		return;
	}
	m_cs->Bind(devCtx);
	// binding resources
	for (auto& iter : m_bindingData) {
		switch (iter.resType)
		{
		case MST_CONSTANT_BUFFER:
			iter.resPointer.constBuf->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_BUFFER:
			iter.resPointer.buf->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_TEXTURE:
			iter.resPointer.tex->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_SAMPLER_STATE:
			iter.resPointer.sampler->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_UNKNOWN_OBJECT:
			iter.resPointer.object->Bind(devCtx, iter.bindTarget, iter.slot);
			break;
		default:
			MLOG(LW, __FUNCTION__, LL, "Unknown binding target!");
			break;
		}
	}
	devCtx->Dispatch(m_groups.x, m_groups.y, m_groups.z);
}

void ComputingPass::End(ID3D11DeviceContext* devCtx) {
	if (!m_cs) return;
	m_cs->Unbind(devCtx);
	// Unbind resource
	for (auto& iter : m_bindingData) {
		switch (iter.resType)
		{
		case MST_CONSTANT_BUFFER:
			iter.resPointer.constBuf->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_BUFFER:
			iter.resPointer.buf->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_TEXTURE:
			iter.resPointer.tex->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_SAMPLER_STATE:
			iter.resPointer.sampler->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		case MST_UNKNOWN_OBJECT:
			iter.resPointer.object->Unbind(devCtx, iter.bindTarget, iter.slot);
			break;
		default:
			MLOG(LW, __FUNCTION__, LL, "Unknown binding target!");
			break;
		}
	}
}
