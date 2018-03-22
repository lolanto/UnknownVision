#include "Camera.h"
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
	m_isViewDirty(true), m_isProjDirty(true) {
	m_buf.GetData().m_pos = desc.position;
	m_buf.GetData().m_param = { m_near, m_far, desc.width, desc.height };
}

////////////////////////////////////////////
// UObject
///////////////////////////////////////////

bool Camera::Setup(ID3D11Device* dev) {
	// calculate data
	calcProjMatrix();
	calcViewMatrix();
	return true;
}

void Camera::Bind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	// bind buffer to shader
	m_buf.Bind(devCtx, sbt, slot);
}

void Camera::Unbind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	m_buf.Unbind(devCtx, sbt, slot);
}

////////////////////////////////////////////////////////////////
// public func
///////////////////////////////////////////////////////////////

XMFLOAT3 Camera::GetPosition() const { return m_buf.ReadData().m_pos; }

XMFLOAT3 Camera::GetLookAt() const { return m_lookAt; }

float Camera::GetFOV() const { return m_fov; }

float Camera::GetASPECT() const { return m_aspect; }

float Camera::GetNear() const { return m_near; }

float Camera::GetFar() const { return m_far; }

XMFLOAT4X4 Camera::GetViewMat() const { return m_buf.ReadData().m_viewMatrix; }
XMFLOAT4X4 Camera::GetProMat() const { return m_buf.ReadData().m_projMatrix; }

XMFLOAT4X4 Camera::GetPrevViewMat() const { return m_cameraPrevData.m_viewMatrix; }
XMFLOAT4X4 Camera::GetPrevProjMat() const { return m_cameraPrevData.m_projMatrix; }

void Camera::SetPosition(XMFLOAT3 pos) {
	m_buf.GetData().m_pos = pos;
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
	// º∆À„Õ∂”∞æÿ’Û
	DirectX::XMMATRIX proMat = DirectX::XMMatrixPerspectiveFovLH(m_fov, m_aspect, m_near, m_far);
	// ¥Ê¥¢ƒÊæÿ’Û
	DirectX::XMStoreFloat4x4(&m_buf.GetData().m_projMatrix, proMat);
	// º∆À„Õ∂”∞æÿ’ÛµƒƒÊæÿ’Û
	DirectX::XMStoreFloat4x4(&m_buf.GetData().m_projMatrixInv, DirectX::XMMatrixInverse(NULL, proMat));
}

void Camera::calcViewMatrix() {
	static XMVECTOR eyePos;
	static XMVECTOR lookAt;
	static XMFLOAT3 tup = XMFLOAT3(0.0f, 1.0f, 0.0);
	static XMVECTOR up = DirectX::XMLoadFloat3(&tup);

	eyePos = DirectX::XMLoadFloat3(&m_buf.ReadData().m_pos);
	lookAt = DirectX::XMLoadFloat3(&m_lookAt);

	DirectX::XMStoreFloat4x4(&m_buf.GetData().m_viewMatrix,
		DirectX::XMMatrixLookAtLH(eyePos, lookAt, up)
	);

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
