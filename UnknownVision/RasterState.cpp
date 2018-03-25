#include "RasterState.h"
#include "InfoLog.h"

RasterState::RasterState(D3D11_CULL_MODE cullMode,
	FrontFaceOrder frontFaceOrder,
	D3D11_FILL_MODE fillMode) {
	// 一切使用默认状态
	m_rasterDesc.AntialiasedLineEnable = FALSE;
	m_rasterDesc.MultisampleEnable = FALSE;
	m_rasterDesc.ScissorEnable = FALSE;
	m_rasterDesc.DepthClipEnable = TRUE;
	m_rasterDesc.DepthBiasClamp = 0.0f;
	m_rasterDesc.SlopeScaledDepthBias = 0.0f;
	m_rasterDesc.DepthBias = 0;
	// 自定义内容
	m_rasterDesc.FillMode = fillMode;
	m_rasterDesc.CullMode = cullMode;
	m_rasterDesc.FrontCounterClockwise = frontFaceOrder;
}

bool RasterState::Setup(ID3D11Device* dev) {
	if (FAILED(dev->CreateRasterizerState(&m_rasterDesc, m_rasterState.ReleaseAndGetAddressOf()))) {
		MLOG(LL, "RasterState::Setup: ", LW, "Create raster state failed!");
		return false;
	}
	return true;
}

void RasterState::Bind(ID3D11DeviceContext* devCtx, ShaderBindTarget, SIZE_T) {
	devCtx->RSSetState(m_rasterState.Get());
}

void RasterState::Unbind(ID3D11DeviceContext* devCtx, ShaderBindTarget, SIZE_T) {
	// Do Nothing!
}
