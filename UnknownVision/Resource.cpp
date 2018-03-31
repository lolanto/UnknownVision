#include "Resource.h"
#include "InfoLog.h"
#include <assert.h>

#define BindParams ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot
#define UnbindParams BindParams

void IConstantBuffer::Bind(BindParams)
{
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetConstantBuffers(slot, 1, m_buf.GetAddressOf());
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetConstantBuffers(slot, 1, m_buf.GetAddressOf());
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetConstantBuffers(slot, 1, m_buf.GetAddressOf());
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetConstantBuffers(slot, 1, m_buf.GetAddressOf());
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "UnBind Constant buffer failed!");
		break;
	}
}

void IConstantBuffer::Unbind(UnbindParams) {
	static ID3D11Buffer* ppNull[] = { nullptr };
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetConstantBuffers(slot, 1, ppNull);
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetConstantBuffers(slot, 1, ppNull);
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetConstantBuffers(slot, 1, ppNull);
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetConstantBuffers(slot, 1, ppNull);
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "Bind Constant buffer failed!");
		break;
	}
}

void IBuffer::Bind(BindParams)
{
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetShaderResources(slot, 1, m_srv_buf.GetAddressOf());
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetShaderResources(slot, 1, m_srv_buf.GetAddressOf());
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetShaderResources(slot, 1, m_srv_buf.GetAddressOf());
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetShaderResources(slot, 1, m_srv_buf.GetAddressOf());
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "Bind buffer failed!");
		break;
	}
}

void IBuffer::Unbind(UnbindParams) {
	static ID3D11ShaderResourceView* ppNull[] = { nullptr };
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetShaderResources(slot, 1, ppNull);
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetShaderResources(slot, 1, ppNull);
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetShaderResources(slot, 1, ppNull);
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetShaderResources(slot, 1, ppNull);
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "UnBind buffer failed!");
		break;
	}
}

void IUnorderAccess::Bind(BindParams)
{
	switch (sbt)
	{
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetUnorderedAccessViews(slot, 1, m_uav.GetAddressOf(), NULL);
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "Bind unorder access resource failed!");
		break;
	}
}

void IUnorderAccess::Unbind(UnbindParams)
{
	static ID3D11UnorderedAccessView* ppNull[] = { nullptr };
	switch (sbt)
	{
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetUnorderedAccessViews(slot, 1, ppNull, NULL);
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "unbind unorder access resource failed!");
		break;
	}

}

void ITexture::Bind(BindParams)
{
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetShaderResources(slot, 1, m_srv_tex.GetAddressOf());
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetShaderResources(slot, 1, m_srv_tex.GetAddressOf());
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetShaderResources(slot, 1, m_srv_tex.GetAddressOf());
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetShaderResources(slot, 1, m_srv_tex.GetAddressOf());
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "Bind texture failed!");
		break;
	}
}

void ITexture::Unbind(UnbindParams) {
	static ID3D11ShaderResourceView* ppNull[] = { nullptr };
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetShaderResources(slot, 1, ppNull);
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetShaderResources(slot, 1, ppNull);
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetShaderResources(slot, 1, ppNull);
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetShaderResources(slot, 1, ppNull);
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "unBind texture failed!");
		break;
	}
}

void ITexture::GenMipMap(ID3D11DeviceContext* devCtx) {
	devCtx->GenerateMips(m_srv_tex.Get());
}

void ISamplerState::Bind(BindParams)
{
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetSamplers(slot, 1, m_sampler.GetAddressOf());
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetSamplers(slot, 1, m_sampler.GetAddressOf());
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetSamplers(slot, 1, m_sampler.GetAddressOf());
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetSamplers(slot, 1, m_sampler.GetAddressOf());
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "bind sampler failed!");
		break;
	}
}

void ISamplerState::Unbind(UnbindParams) {
	static ID3D11SamplerState* ppNull[] = { nullptr };
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		devCtx->VSSetSamplers(slot, 1, ppNull);
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetSamplers(slot, 1, ppNull);
		break;
	case SBT_GEOMETRY_SHADER:
		devCtx->GSSetSamplers(slot, 1, ppNull);
		break;
	case SBT_COMPUTE_SHADER:
		devCtx->CSSetSamplers(slot, 1, ppNull);
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "bind sampler failed!");
		break;
	}
}

ID3D11RenderTargetView* IRenderTarget::GetRTV() { 
	assert(m_rtv.Get() != NULL);
	return m_rtv.Get();
}

ID3D11DepthStencilView* IDepthStencil::GetDSV() {
	assert(m_dsv.Get() != NULL);
	return m_dsv.Get();
}

RenderTargetWrapper::RenderTargetWrapper(ID3D11RenderTargetView* rtv) {
	m_rtv.Attach(rtv);
}
