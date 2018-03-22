#pragma once
#include <wrl.h>
#include "UnknownObject.h"

class Sampler : public UnknownObject {
public:
	Sampler(D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	Sampler(D3D11_SAMPLER_DESC desc);
public:
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);

	void SetSlot(UINT);
	void SetTarget(ShaderBindTarget);

private:
	D3D11_SAMPLER_DESC												m_desc;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_sampler;

	UINT																			m_slot;
	ShaderBindTarget															m_target;

	bool																				m_isBinding;
};