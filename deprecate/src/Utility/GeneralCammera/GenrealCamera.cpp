#include "GeneralCamera.h"
using namespace UVCameraUtility;
#define USING_DXLIKE
#ifdef USING_DXLIKE
#include <DirectXMath.h>
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using DirectX::XMVECTOR;
using DirectX::XMMATRIX;

XMVECTOR toVector(const MFLOAT3& input) {
	XMFLOAT3 f3(input.data());
	return DirectX::XMLoadFloat3(&f3);
}

MFLOAT3 toFLOAT3(const XMVECTOR& input) {
	XMFLOAT3 f3;
	DirectX::XMStoreFloat3(&f3, input);
	return { f3.x, f3.y, f3.z };
}

MFLOAT4X4 toFloat4x4(const XMMATRIX& input) {
	DirectX::XMFLOAT4X4 mat;
	DirectX::XMStoreFloat4x4(&mat, input);
	MFLOAT4X4 ret;
	memcpy(ret.data(), &mat, sizeof(float) * 16);
	return ret;
}

MFLOAT3 Cross(const MFLOAT3& a, const MFLOAT3& b) {
	XMVECTOR res = DirectX::XMVector3Cross(toVector(a), toVector(b));
	return toFLOAT3(res);
}

MFLOAT3 Normalize(const MFLOAT3& input) {
	auto&& vec = toVector(input);
	vec = DirectX::XMVector3Normalize(vec);
	return toFLOAT3(vec);
}

MFLOAT3 RotateAroundAxis(const MFLOAT3& input, const MFLOAT3& axis, float radiance) {
	auto&& input_vec = toVector(input);
	auto&& axis_vec = toVector(axis);
	XMVECTOR quaternion = DirectX::XMQuaternionRotationAxis(axis_vec, radiance);
	input_vec = DirectX::XMVector3Rotate(input_vec, quaternion);
	return toFLOAT3(input_vec);
}

std::vector<MFLOAT3> RotateAroundAxis(const std::vector<MFLOAT3>& inputs, const MFLOAT3& axis, float radiance) {
	auto&& axis_vec = toVector(axis);
	XMVECTOR quaternion = DirectX::XMQuaternionRotationAxis(axis_vec, radiance);
	std::vector<MFLOAT3> output; output.reserve(inputs.size());
	for (const auto& input : inputs) {
		auto&& input_vec = toVector(input);
		input_vec = DirectX::XMVector3Rotate(input_vec, quaternion);
		output.push_back(toFLOAT3(input_vec));
	}
	return output;
}

#endif // USING_DXMATH

MFLOAT3 operator-(const MFLOAT3& lhs, const MFLOAT3& rhs) {
	return { lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2] };
}

MFLOAT3 operator+(const MFLOAT3& lhs, const MFLOAT3& rhs) {
	return { lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2] };
}

MFLOAT2 operator-(const MFLOAT2& lhs, const MFLOAT2& rhs) {
	return { lhs[0] - rhs[0], lhs[1] - rhs[1] };
}

GeneralCamera::GeneralCamera(CAMERA_DESC& desc)
	: m_fov(desc.fov), m_width(desc.width), m_height(desc.height),
	m_near(desc.nearPlane), m_far(desc.farPlane),
	m_isViewDirty(true), m_isProjDirty(true) {
	m_position = desc.position;
	MFLOAT3 side = Cross((desc.lookAt - m_position), DEFAULT_UP_DIR);
	m_forward = Normalize(Cross(DEFAULT_UP_DIR, side));
}

////////////////////////////////////////////////////////////////
// public func
///////////////////////////////////////////////////////////////

void GeneralCamera::GetPosition(float* output) const { memcpy(output, m_position.data(), sizeof(float) * 3); }

void GeneralCamera::GetForwardDirection(float* output) const { memcpy(output, m_forward.data(), sizeof(float) * 3); }

void GeneralCamera::GetRightDirection(float* output) const {
#ifdef USING_DXLIKE
	MFLOAT3 right = Cross(DEFAULT_UP_DIR, m_forward);
#endif // USING_DXLIKE
	memcpy(output, right.data(), sizeof(float) * 3);
}

void GeneralCamera::GetUpDirection(float* output) const {
#ifdef USING_DXLIKE
	MFLOAT3 right = Cross(DEFAULT_UP_DIR, m_forward);
	MFLOAT3 up = Cross(m_forward, right);
#endif // USING_DXLIKE
	memcpy(output, up.data(), sizeof(float) * 3);
}

float GeneralCamera::GetFOV() const { return m_fov; }

float GeneralCamera::GetASPECT() const { return m_width / m_height; }

float GeneralCamera::GetNear() const { return m_near; }

float GeneralCamera::GetFar() const { return m_far; }

void GeneralCamera::GetViewMat(float* output) { 
#ifdef USING_DXLIKE
	if (m_isViewDirty) {
		auto&& mat = DirectX::XMMatrixLookAtLH(toVector(m_position),
			toVector(m_position + m_forward),
			toVector(DEFAULT_UP_DIR));
		m_viewMat = toFloat4x4(mat);
		m_isViewDirty = false;
	}
#endif // USING_DXLIKE
	memcpy(output, m_viewMat.data(), sizeof(float) * 16);
}

void GeneralCamera::GetProMat(float* output) {
#ifdef USING_DXLIKE
	if (m_isProjDirty) {
		auto&& mat = DirectX::XMMatrixPerspectiveFovLH(m_fov, GetASPECT(), m_near, m_far);
		m_projMat = toFloat4x4(mat);
		m_isProjDirty = false;
	}
#endif // USING_DXLIKE
	memcpy(output, m_projMat.data(), sizeof(float) * 16);
}

void GeneralCamera::GetLastFrameViewMat(float* output) const
{
	memcpy(output, m_last_viewMat.data(), sizeof(float) * 16);
}

void GeneralCamera::GetLastFrameProjMat(float* output) const
{
	memcpy(output, m_last_projMat.data(), sizeof(float) * 16);
}

void GeneralCamera::SetPosition(float* pos) {
	memcpy(m_position.data(), pos, sizeof(float) * 3);
	m_isViewDirty = true;
}

void GeneralCamera::SetForward(float* input) {
	memcpy(m_forward.data(), input, sizeof(float) * 3);
	m_isViewDirty = true;
}

void GeneralCamera::SetFOV(float f) {
	m_fov = f;
	m_isProjDirty = true;
}

void GeneralCamera::SetHeight(float x) {
	m_height = x;
	m_isProjDirty = true;
}

void GeneralCamera::SetWidth(float x) {
	m_width = x;
	m_isProjDirty = true;
}

void GeneralCamera::UpdatePerFrameEnd() {
	GetViewMat(m_last_viewMat.data());
	GetProMat(m_last_projMat.data());
}

/** EPIC CAMERA CONTROLLER */

EpicCameraController::EpicCameraController(GeneralCamera& camera) : CameraController(camera)
{
	m_camera.GetRightDirection(m_right.data());
	m_camera.GetUpDirection(m_up.data());
	m_camera.GetForwardDirection(m_forward.data());
	m_camera.GetPosition(m_pos.data());

	m_mouseLastPos = { FLT_MAX, FLT_MAX };
	m_isLeftDown = false;
	m_isRightDown = false;
	m_isMidDown = false;
	m_isShiftDown = false;
}

void EpicCameraController::MouseMove(float x, float y)
{
	/** 状态初始化 */
	if (m_mouseLastPos[0] == FLT_MAX && m_mouseLastPos[1] == FLT_MAX) {
		m_mouseLastPos = { x, y };
		return;
	}
	MFLOAT2 deltaRadiance = { (x - m_mouseLastPos[0]) * MOUSE_MOVE_SCALE, (y - m_mouseLastPos[1]) * MOUSE_MOVE_SCALE };
	MFLOAT2 deltaDistance = { (x - m_mouseLastPos[0]) / 100.0f, (y - m_mouseLastPos[1]) / 100.0f };
	m_mouseLastPos = { x, y };
	if (m_isLeftDown) {
		/** 鼠标左键按着	 摄像机前进后退
		 * 往上是摄像机朝forward投影到xz平面方向移动，往后则后移
		 * 向左是朝左边旋转，绕y轴逆时针旋转，向右是绕y轴顺时针旋转 */
		auto&& newCameraAxises = RotateAroundAxis({ m_forward, m_right, m_up }, DEFAULT_UP_DIR, deltaRadiance[0]);

		m_forward = newCameraAxises[0];
		m_right = newCameraAxises[1];
		m_up = newCameraAxises[2];

		MFLOAT3 dir = Normalize({ m_forward[0], 0.0f, m_forward[2] });
		dir[0] *= -deltaDistance[1];
		dir[1] *= -deltaDistance[1];
		dir[2] *= -deltaDistance[1];
		m_pos = m_pos + dir;

		m_camera.SetForward(m_forward.data());
		m_camera.SetPosition(m_pos.data());
	}
	else if (m_isRightDown) {
		/** 鼠标右键控制摄像机的朝向，在原地不动，旋转摄像机
		 * 向上是抬头，绕right轴逆时针旋转，向下是低头，绕right轴顺时针旋转
		 * 向左是朝左边旋转，绕y轴逆时针旋转，向右是绕y轴顺时针旋转 */
		auto&& newCameraAxises_y = RotateAroundAxis({ m_forward, m_right, m_up }, DEFAULT_UP_DIR, deltaRadiance[0]);
		auto&& newCameraAxises_right = RotateAroundAxis({ newCameraAxises_y[0], newCameraAxises_y[2] }, newCameraAxises_y[1], deltaRadiance[1]);
		m_right = newCameraAxises_y[1];
		m_forward = newCameraAxises_right[0];
		m_up = newCameraAxises_right[1];
		m_camera.SetForward(m_forward.data());
	}
	else if (m_isMidDown) {
		/** 鼠标中键控制摄像机在进行上下左右平移
		 * 上下是沿着y轴移动
		 * 左右是沿着自己的right轴移动 */
		m_pos[1] += -deltaDistance[1];
		m_pos = m_pos + MFLOAT3{ m_right[0] * deltaDistance[0], m_right[1] * deltaDistance[0], m_right[2] * deltaDistance[0] };

		m_camera.SetPosition(m_pos.data());
	}
}

void EpicCameraController::MouseEventHandler(MouseButton btn, bool state) {
	switch (btn) {
	case MBTN_LEFT:
		m_isLeftDown = state;
		break;
	case MBTN_RIGHT:
		m_isRightDown = state;
		break;
	case MBTN_MID:
		m_isMidDown = state;
		break;
	default:
		break;
	}
}

void EpicCameraController::KeyEventHandler(KeyButton btn, bool state) {
	if (btn == KBTN_SHIFT) m_isShiftDown = state;
}
