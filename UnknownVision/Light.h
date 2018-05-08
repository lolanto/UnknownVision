#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include "UnknownObject.h"
#include "Buffer.h"


class SpotLight : public UnknownObject{
	struct BufData {
		// xyz: light world position; w: cone inside angle
		DirectX::XMFLOAT4							posAndInside;
		DirectX::XMFLOAT4							color;
		// xyz: light world orient; w: cone outside angle
		DirectX::XMFLOAT4							orientAndOutside;
		DirectX::XMFLOAT4X4						viewMatrix;
		DirectX::XMFLOAT4X4						projMatrix;
	};
public:
	SpotLight(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 color,
		DirectX::XMFLOAT3 orient, float insideAngle, float outsideAngle);
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	// need some function to update parameters
	void SetDepthNearFar(float n = -1, float f = -1);
private:
	void calcViewMatrix();
	void calcProjMatrix();
private:
	float														m_depthTexNear;
	float														m_depthTexFar;
	ConstantBuffer<SpotLight::BufData>	m_buf;
};

