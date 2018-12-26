#pragma once
#include <wrl.h>
#include "UnknownObject.h"
#include <DirectXMath.h>

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
	void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
public:
	D3D11_RASTERIZER_DESC													m_rasterDesc;
private:
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>				m_rasterState;
};


class BlendState : public UnknownObject {
public:
	BlendState(D3D11_BLEND_DESC desc, DirectX::XMFLOAT4 factor = { 1.0f, 1.0f, 1.0f, 1.0f }, UINT mask = UINT_MAX);

	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*, ShaderBindTarget sbt = SBT_UNKNOWN, SIZE_T slot = 0);
	void Unbind(ID3D11DeviceContext*, ShaderBindTarget sbt = SBT_UNKNOWN, SIZE_T slot = 0);

public:
	const D3D11_BLEND_DESC													BlendStateDesc;
	const DirectX::XMFLOAT4														BlendFactor;
	const UINT																			BlendMask;
private:
	Microsoft::WRL::ComPtr<ID3D11BlendState>						m_blendState;
};
