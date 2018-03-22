
#include "Model.h"
#include "InfoLog.h"
#include "Pipeline.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;
using DirectX::XMFLOAT4X4;
using std::vector;

void Model::defConstruct() {
	m_pos = XMFLOAT3(0, 0, 0);
	m_rotateOrig = XMFLOAT3(0, 0, 0);
	m_isDirty = true;
	m_hasSetup = false;
	m_slot = 1;
}

Model::Model() {
	defConstruct();
}

///////////////////
// public function
///////////////////

bool Model::Setup(ID3D11Device* dev) {
	if (m_hasSetup) return true;
	// Create buffer
	calcModelMatrix();
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = sizeof(m_modelData);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	subData.pSysMem = &m_modelData;
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	if (FAILED(dev->CreateBuffer(&desc, &subData, m_buf.ReleaseAndGetAddressOf()))) {
		MLOG(LL, "Model::Setup: create buffer failed!");
		return false;
	}
	m_hasSetup = true;
	return true;
}

void Model::Bind(ID3D11DeviceContext* devCtx) {
	if (!m_hasSetup) {
		MLOG(LW, "Model::Bind: model has not setup !");
		return;
	}
	if (m_isDirty) {
		calcModelMatrix();
		D3D11_MAPPED_SUBRESOURCE map;
		if (FAILED(devCtx->Map(m_buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) {
			MLOG(LW, "Model::Bind: update model matrix failed! map failed!");
		}
		else {
			memcpy_s(map.pData, sizeof(m_modelData), 
				&m_modelData, sizeof(m_modelData));
			devCtx->Unmap(m_buf.Get(), 0);
		}
	}
	devCtx->VSSetConstantBuffers(m_slot, 1, m_buf.GetAddressOf());
}

void Model::Unbind(ID3D11DeviceContext* devCtx) {
	return;
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
	return m_modelData;
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
	DirectX::XMStoreFloat4x4(&m_modelData.modelMatrix, translateMat);
	// ¼ÆËãÄæ¾ØÕó
	translateMat = DirectX::XMMatrixInverse(NULL, translateMat);
	DirectX::XMStoreFloat4x4(&m_modelData.modelMatrixInv, translateMat);
	m_isDirty = false;
}
