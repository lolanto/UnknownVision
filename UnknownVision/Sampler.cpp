#include "Sampler.h"
#include "InfoLog.h"

Sampler::Sampler(D3D11_FILTER filter)
{
	m_desc.Filter = filter;
	m_desc.AddressU = 
		m_desc.AddressV = 
		m_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	m_desc.MinLOD =
		m_desc.MipLODBias = 0;
	m_desc.MaxLOD = D3D11_FLOAT32_MAX;
}

Sampler::Sampler(D3D11_SAMPLER_DESC desc)
	:m_desc(desc) {}

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

