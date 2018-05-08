#pragma once
#include <wrl.h>
#include "Resource.h"

class Sampler : public ISamplerState {
public:
	Sampler(D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR, 
		D3D11_TEXTURE_ADDRESS_MODE texAddMode = D3D11_TEXTURE_ADDRESS_WRAP);
	Sampler(D3D11_SAMPLER_DESC desc);
public:
	bool Setup(ID3D11Device*);
private:
	D3D11_SAMPLER_DESC												m_desc;
};