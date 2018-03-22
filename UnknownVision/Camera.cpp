#include "Camera.h"
#include "RendererProxy.h"
#include "InfoLog.h"

using DirectX::XMFLOAT4X4;
using DirectX::XMFLOAT3;
using DirectX::XMVECTOR;

CAMERA_DESC::CAMERA_DESC(float width, float height, DirectX::XMFLOAT3 pos,	DirectX::XMFLOAT3 lkt, float fv, float np, float fp) 
	: width(width), height(height), position(pos), lookAt(lkt), fov(fv), nearPlane(np), farPlane(fp)
{}

Camera::Camera(CAMERA_DESC& desc)
	: m_fov(desc.fov), m_aspect(desc.width / desc.height),
	m_lookAt(desc.lookAt), m_near(desc.nearPlane), m_far(desc.farPlane),
	m_isViewDirty(true), m_isProjDirty(true), m_hasSetup(false), m_VSSlot(2), m_PSSlot(0) {
	m_cameraDataStruct.m_pos = desc.position;
	m_cameraDataStruct.m_param = { m_near, m_far, desc.width, desc.height };
	m_vpMatrixdata = std::shared_ptr<byte>(new byte[sizeof(m_cameraDataStruct)]);
	// 设置m_param
	memcpy_s(m_vpMatrixdata.get() + offsetof(CameraDataStruct, m_param), sizeof(m_cameraDataStruct.m_param),
		&m_cameraDataStruct.m_param, sizeof(m_cameraDataStruct.m_param));
}

////////////////////////////////////////////
// UObject
///////////////////////////////////////////

bool Camera::Setup(ID3D11Device* dev) {
	if (m_hasSetup) return true;
	// calculate data
	calcProjMatrix();
	calcViewMatrix();
	// create buffers
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	HRESULT hr;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	desc.ByteWidth = sizeof(m_cameraDataStruct);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	subData.pSysMem = m_vpMatrixdata.get();
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	hr = dev->CreateBuffer(&desc, &subData, m_ccbuf.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, "Camera::Setup: create buffer failed!");
		return false;
	}
	m_hasSetup = true;
	return true;
}

void Camera::Bind(ID3D11DeviceContext* devCtx) {
	// check if need to update buffer data
	if (m_isViewDirty || m_isProjDirty) {
		// 矩阵信息需要更新，备份当前信息
		m_cameraPrevData = m_cameraDataStruct;
		if (m_isViewDirty) calcViewMatrix();
		if (m_isProjDirty) calcProjMatrix();
		// map buffer
		D3D11_MAPPED_SUBRESOURCE ms;
		if (devCtx->Map(m_ccbuf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)) {
			MLOG(LL, "Camera::Bind: map buffer failed!");
			return;
		}
		memcpy_s(ms.pData, sizeof(m_cameraDataStruct),
			m_vpMatrixdata.get(), sizeof(m_cameraDataStruct));
		devCtx->Unmap(m_ccbuf.Get(), 0);
	}
	// bind buffer to shader
	devCtx->VSSetConstantBuffers(m_VSSlot, 1, m_ccbuf.GetAddressOf());
	devCtx->PSSetConstantBuffers(m_PSSlot, 1, m_ccbuf.GetAddressOf());
}

void Camera::Unbind(ID3D11DeviceContext* devCtx) {
	ID3D11Buffer* tmpBuf[] = { NULL };
	devCtx->VSSetConstantBuffers(m_VSSlot, 1, tmpBuf);
	devCtx->PSSetConstantBuffers(m_PSSlot, 1, tmpBuf);
}

////////////////////////////////////////////////////////////////
// public func
///////////////////////////////////////////////////////////////

XMFLOAT3 Camera::GetPosition() const { return m_cameraDataStruct.m_pos; }

XMFLOAT3 Camera::GetLookAt() const { return m_lookAt; }

float Camera::GetFOV() const { return m_fov; }

float Camera::GetASPECT() const { return m_aspect; }

float Camera::GetNear() const { return m_near; }

float Camera::GetFar() const { return m_far; }

XMFLOAT4X4 Camera::GetViewMat() const { return m_cameraDataStruct.m_viewMatrix; }
XMFLOAT4X4 Camera::GetProMat() const { return m_cameraDataStruct.m_projMatrix; }

XMFLOAT4X4 Camera::GetPrevViewMat() const { return m_cameraPrevData.m_viewMatrix; }
XMFLOAT4X4 Camera::GetPrevProjMat() const { return m_cameraPrevData.m_projMatrix; }

void Camera::SetPosition(XMFLOAT3 pos) {
	m_cameraDataStruct.m_pos = pos;
	m_isViewDirty = true;
}

void Camera::SetLookAt(XMFLOAT3 lookAt) {
	m_lookAt = lookAt;
	m_isViewDirty = true;
}

void Camera::SetFOV(float f) {
	m_fov = f;
	m_isProjDirty = true;
}

void Camera::SetASPECT(float a) {
	m_aspect = a;
	m_isProjDirty = true;
}

////////////////////////////////////////////////////////////////
// private func
///////////////////////////////////////////////////////////////

void Camera::calcProjMatrix() {
	// 计算投影矩阵
	DirectX::XMMATRIX proMat = DirectX::XMMatrixPerspectiveFovLH(m_fov, m_aspect, m_near, m_far);
	// 存储逆矩阵
	DirectX::XMStoreFloat4x4(&m_cameraDataStruct.m_projMatrix, proMat);
	// 计算投影矩阵的逆矩阵
	DirectX::XMStoreFloat4x4(&m_cameraDataStruct.m_projMatrixInv, DirectX::XMMatrixInverse(NULL, proMat));

	memcpy_s(m_vpMatrixdata.get() + offsetof(CameraDataStruct, m_projMatrix), sizeof(m_cameraDataStruct.m_projMatrix),
		&m_cameraDataStruct.m_projMatrix, sizeof(m_cameraDataStruct.m_projMatrix));
	memcpy_s(m_vpMatrixdata.get() + offsetof(CameraDataStruct, m_projMatrixInv), sizeof(m_cameraDataStruct.m_projMatrixInv),
		&m_cameraDataStruct.m_projMatrixInv, sizeof(m_cameraDataStruct.m_projMatrixInv));
	m_isProjDirty = false;
}

void Camera::calcViewMatrix() {
	static XMVECTOR eyePos;
	static XMVECTOR lookAt;
	static XMFLOAT3 tup = XMFLOAT3(0.0f, 1.0f, 0.0);
	static XMVECTOR up = DirectX::XMLoadFloat3(&tup);

	eyePos = DirectX::XMLoadFloat3(&m_cameraDataStruct.m_pos);
	lookAt = DirectX::XMLoadFloat3(&m_lookAt);

	DirectX::XMStoreFloat4x4(&m_cameraDataStruct.m_viewMatrix,
		DirectX::XMMatrixLookAtLH(eyePos, lookAt, up)
	);
	memcpy_s(m_vpMatrixdata.get(), sizeof(m_cameraDataStruct.m_viewMatrix),
		&m_cameraDataStruct.m_viewMatrix, sizeof(m_cameraDataStruct.m_viewMatrix));
	memcpy_s(m_vpMatrixdata.get() + offsetof(CameraDataStruct, m_pos), sizeof(m_cameraDataStruct.m_pos),
		&m_cameraDataStruct.m_pos, sizeof(m_cameraDataStruct.m_pos));
	m_isViewDirty = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   cubeMap Helper   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CubeMapHelper::CubeMapHelper(DirectX::XMFLOAT3 pos, float nearZ, float farZ)
	: m_nearZ(nearZ), m_farZ(farZ) {
	// generate view matrix
	DirectX::XMVECTOR eyePos = DirectX::XMLoadFloat3(&pos);
	DirectX::XMFLOAT3 dir = { 0, 0, 1 };
	DirectX::XMVECTOR focus = DirectX::XMVectorAdd(eyePos, DirectX::XMLoadFloat3(&dir));
	DirectX::XMFLOAT3 upDir = { 0, 1, 0 };
	DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&upDir);
	m_viewMat = DirectX::XMMatrixLookAtLH(eyePos, focus, up);

	// generate projection matrix
	// fov: top-down filed-of-view
	m_projectMat = DirectX::XMMatrixPerspectiveFovLH(1.57f, 1.0f, m_nearZ, m_farZ);

	// generate rotation matrix
	m_leftMat = DirectX::XMMatrixRotationY(-1.57f);
	m_rightMat = DirectX::XMMatrixRotationY(1.57f);
	m_topMat = DirectX::XMMatrixRotationX(1.57f);
	m_bottomMat = DirectX::XMMatrixRotationX(-1.57f);
	m_backMat = DirectX::XMMatrixRotationY(3.14f);
}

void CubeMapHelper::GetBasicViewMat(DirectX::XMFLOAT4X4* output) {
	DirectX::XMStoreFloat4x4(output, m_viewMat);
}

void CubeMapHelper::GetRotAndProjMat(DirectX::XMFLOAT4X4* outputs) {
	DirectX::XMStoreFloat4x4(outputs, m_leftMat);
	DirectX::XMStoreFloat4x4(outputs + 1, m_rightMat);
	DirectX::XMStoreFloat4x4(outputs + 2, m_topMat);
	DirectX::XMStoreFloat4x4(outputs + 3, m_bottomMat);
	DirectX::XMStoreFloat4x4(outputs + 4, m_frontMat);
	DirectX::XMStoreFloat4x4(outputs + 5, m_backMat);
	DirectX::XMStoreFloat4x4(outputs + 6, m_projectMat);
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   CameraController   ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CameraController::CameraController(Camera* c) : m_camera(c) {}
