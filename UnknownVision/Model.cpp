
#include "Model.h"
#include "InfoLog.h"
#include "Pipeline.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;
using DirectX::XMFLOAT4X4;
using std::vector;

Model::Model(XMFLOAT3 pos, XMFLOAT3 rotate)
	: m_pos(pos), m_rotateOrig(rotate), m_isDirty(true) {}

///////////////////
// public function
///////////////////

bool Model::Setup(ID3D11Device* dev) {
	// Create buffer
	calcModelMatrix();
	m_buf.Setup(dev);
	return true;
}

void Model::Bind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	m_buf.Bind(devCtx, sbt, slot);
}

void Model::Unbind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	m_buf.Unbind(devCtx, sbt, slot);
}

/////////////////////////////////
// Public Func
/////////////////////////////////

void Model::Translate(XMFLOAT3& dir) {
	static DirectX::XMVECTOR v1, v2;
	v1 = DirectX::XMLoadFloat3(&dir);
	v2 = DirectX::XMLoadFloat3(&m_pos);
	v1 = DirectX::XMVectorAdd(v1, v2);
	DirectX::XMStoreFloat3(&m_pos, v1);
	m_isDirty = true;
}

void Model::RotateAroundOrigin(XMFLOAT3& angle) {
	m_rotateOrig.x += angle.x;
	m_rotateOrig.y += angle.y;
	m_rotateOrig.z += angle.z;
	m_isDirty = true;
}

ModelData Model::GetModelData() const {
	return m_buf.ReadData();
}

///////////////////
// private function
///////////////////

void Model::calcModelMatrix() {
	DirectX::XMMATRIX translateMat = DirectX::XMMatrixTranslation(
		m_pos.x,
		m_pos.y,
		m_pos.z);
	DirectX::XMMATRIX rotateMat = DirectX::XMMatrixRotationRollPitchYaw(
		m_rotateOrig.x,
		m_rotateOrig.y,
		m_rotateOrig.z
	);
	translateMat = DirectX::XMMatrixMultiply(translateMat, rotateMat);
	DirectX::XMStoreFloat4x4(&m_buf.GetData().modelMatrix, translateMat);
	// ¼ÆËãÄæ¾ØÕó
	translateMat = DirectX::XMMatrixInverse(NULL, translateMat);
	DirectX::XMStoreFloat4x4(&m_buf.GetData().modelMatrixInv, translateMat);
	m_isDirty = false;
}
