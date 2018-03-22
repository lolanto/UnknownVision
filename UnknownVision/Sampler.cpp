#include "Sampler.h"
#include "InfoLog.h"

Sampler::Sampler(D3D11_FILTER filter)
	: m_slot(-1), m_target(SBT_UNKNOWN), m_isBinding(false) {
	m_desc.Filter = filter;
	m_desc.AddressU = 
		m_desc.AddressV = 
		m_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	m_desc.MinLOD =
		m_desc.MipLODBias = 0;
	m_desc.MaxLOD = D3D11_FLOAT32_MAX;
}

Sampler::Sampler(D3D11_SAMPLER_DESC desc)
	: m_slot(-1), m_target(SBT_UNKNOWN), m_isBinding(false),
	m_desc(desc) {}

///////////////////
// public function
///////////////////
bool Sampler::Setup(ID3D11Device* dev) {
	if (FAILED(dev->CreateSamplerState(&m_desc, m_sampler.ReleaseAndGetAddressOf()))) {
		MLOG(LL, "Sampler::Setup: create sampler state failed!");
		return false;
	}
	return true;
}

void Sampler::Bind(ID3D11DeviceContext* devCtx) {
	if (m_isBinding) {
		MLOG(LW, "Sampler::Bind: sampler is binding!");
		return;
	}
	switch (m_target) {
	case SBT_VERTEX_SHADER:
		devCtx->VSSetSamplers(m_slot, 1, m_sampler.GetAddressOf());
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetSamplers(m_slot, 1, m_sampler.GetAddressOf());
		break;
	default:
		MLOG(LE, "Sampler::Bind: binding target invalid!");
		return;
	}
	m_isBinding = true;
}

void Sampler::Unbind(ID3D11DeviceContext* devCtx) {
	static ID3D11SamplerState* NULLPTR[] = { NULL };
	switch (m_target) {
	case SBT_VERTEX_SHADER:
		devCtx->VSSetSamplers(m_slot, 1, NULLPTR);
		break;
	case SBT_PIXEL_SHADER:
		devCtx->PSSetSamplers(m_slot, 1, NULLPTR);
		break;
	}
	m_isBinding = false;
}

void Sampler::SetSlot(UINT slot) {
	if (m_isBinding) {
		MLOG(LW, "Sampler::SetSlot: sampler is binding can not change slot now!");
		return;
	}
	m_slot = slot; 
}

void Sampler::SetTarget(ShaderBindTarget sbt) {
	if (m_isBinding) {
		MLOG(LW, "Sampler::SetTarget: sampler is binding can not change binding target now!");
		return;
	}
	m_target = sbt;
}
