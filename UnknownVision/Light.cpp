#include "Light.h"
#include "InfoLog.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   SpotLight   ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

SpotLight::SpotLight(XMFLOAT3 pos, XMFLOAT3 color,
	XMFLOAT3 orient, float insideAngle, float outsideAngle) {
	SpotLight::BufData& bufRef = m_buf.GetData();
	bufRef.posAndInside = { pos.x, pos.y, pos.z, insideAngle };
	bufRef.color = { color.x, color.y, color.z, 0 };
	bufRef.orientAndOutside = { orient.x, orient.y, orient.z, outsideAngle };
	m_depthTexNear = 0.1f;
	m_depthTexFar = 50.0f;
}

bool SpotLight::Setup(ID3D11Device* dev) {
	calcViewMatrix();
	calcProjMatrix();
	return m_buf.Setup(dev);
}

void SpotLight::Bind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	m_buf.Bind(devCtx, sbt, slot);
}

void SpotLight::Unbind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	m_buf.Unbind(devCtx, sbt, slot);
}

void SpotLight::SetDepthNearFar(float n, float f) {
	if (n > 0) m_depthTexNear = n;
	if (f > 0) m_depthTexFar = f;
}

///////////////////
// private function
///////////////////
void SpotLight::calcViewMatrix() {
	static DirectX::XMMATRIX temp;
	static DirectX::XMFLOAT3 up;
	static DirectX::XMFLOAT3 foucus;
	static DirectX::XMFLOAT3 pos;

	const SpotLight::BufData& bufRef = m_buf.ReadData();

	pos = DirectX::XMFLOAT3(bufRef.posAndInside.x, bufRef.posAndInside.y, bufRef.posAndInside.z);

	up = DirectX::XMFLOAT3(0, 0, 1);
	if (up.x * bufRef.orientAndOutside.x + up.y * bufRef.orientAndOutside.y + up.z * bufRef.orientAndOutside.z) {
		up.z += 0.1f;
	}

	foucus = DirectX::XMFLOAT3(
		bufRef.posAndInside.x + bufRef.orientAndOutside.x,
		bufRef.posAndInside.y + bufRef.orientAndOutside.y,
		bufRef.posAndInside.z + bufRef.orientAndOutside.z
	);

	temp = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&pos),
		DirectX::XMLoadFloat3(&foucus),
		DirectX::XMLoadFloat3(&up)
	);

	DirectX::XMStoreFloat4x4(&m_buf.GetData().viewMatrix, temp);
}

void SpotLight::calcProjMatrix() {
	static DirectX::XMMATRIX temp;

	const SpotLight::BufData& bufRef = m_buf.ReadData();

	temp = DirectX::XMMatrixPerspectiveFovLH(bufRef.orientAndOutside.w, 1, m_depthTexNear, m_depthTexFar);
	DirectX::XMStoreFloat4x4(&m_buf.GetData().projMatrix, temp);
}