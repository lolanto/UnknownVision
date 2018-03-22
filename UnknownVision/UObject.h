#pragma once
// Object Interface

#include <d3d11.h>

class UObject {
public:
	virtual bool Setup(ID3D11Device*, ID3D11DeviceContext*) = 0;
	virtual void Bind(ID3D11Device*, ID3D11DeviceContext*) = 0;
	virtual void Unbind(ID3D11Device*, ID3D11DeviceContext*) = 0;
};
