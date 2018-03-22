#pragma once
#include <wrl.h>
#include "UnknownObject.h"

enum FrontFaceOrder {
	CLOCK = 0,
	COUNTERCLOCK = 1
};

class RasterState : public UnknownObject {
public:
	RasterState(D3D11_CULL_MODE cullMode = D3D11_CULL_BACK, 
		FrontFaceOrder frontFaceOrder = CLOCK,
		D3D11_FILL_MODE fillMode = D3D11_FILL_SOLID);

	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);
public:
	D3D11_RASTERIZER_DESC													m_rasterDesc;
private:
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>				m_rasterState;
};
