#include "Camera.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;
using DirectX::XMVECTOR;

const float MOUSE_MOVE_SCALE = 0.005f;

OrbitController::OrbitController(Camera* c)
	: CameraController(c) {
	// 根据camera基本属性初始化控制器变量
	XMFLOAT3 tmp1;
	XMFLOAT3 tmp2;
	// set forward vector
	tmp1 = c->GetLookAt();
	tmp2 = c->GetPosition();
	tmp1.x = tmp1.x - tmp2.x;
	tmp1.y = tmp1.y - tmp2.y;
	tmp1.z = tmp1.z - tmp2.z;
	m_forward = DirectX::XMLoadFloat3(&tmp1);
	// set left
	tmp1 = XMFLOAT3(0, 1, 0);
	m_left = DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&tmp1), m_forward);
	m_left = DirectX::XMVector3Normalize(m_left);
	// set up
	m_up = DirectX::XMVector3Cross(m_forward, m_left);
	m_up = DirectX::XMVector3Normalize(m_up);
	// set pos
	m_pos = DirectX::XMLoadFloat3(&c->GetPosition());

	m_mouseLastPos.x = FLT_MAX;
	m_mouseLastPos.y = FLT_MAX;
	m_isLeftDown = false;
	m_isRightDown = false;
}

///////////////////
// public function
///////////////////

void OrbitController::MouseMove(float x, float y) {
	if (m_mouseLastPos.x == FLT_MAX) {
		m_mouseLastPos = XMFLOAT2(x, y);
		return;
	}
	static XMFLOAT2 delta;
	static XMVECTOR quaternion;
	static XMFLOAT3 tmpVec3;
	delta.x = (x - m_mouseLastPos.x) * MOUSE_MOVE_SCALE;
	//delta.x = (m_mouseLastPos.x - x) * MOUSE_MOVE_SCALE;
	delta.y = (y - m_mouseLastPos.y) * MOUSE_MOVE_SCALE;
	m_mouseLastPos.x = x;
	m_mouseLastPos.y = y;
	if (m_isLeftDown) {
		// 鼠标左键按着	
		// forward以position为中心，绕Y轴旋转x度
		tmpVec3 = DirectX::XMFLOAT3(0, 1, 0);
		quaternion = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(&tmpVec3), delta.x);
		m_forward = DirectX::XMVector3Rotate(m_forward, quaternion);
		m_left = DirectX::XMVector3Rotate(m_left, quaternion);
		m_up = DirectX::XMVector3Rotate(m_up, quaternion);
		// 绕left轴渲染y度
		quaternion = DirectX::XMQuaternionRotationAxis(m_left, delta.y);
		m_forward = DirectX::XMVector3Rotate(m_forward, quaternion);
		m_up = DirectX::XMVector3Rotate(m_up, quaternion);
		// update! position!
		quaternion = DirectX::XMLoadFloat3(&m_camera->GetLookAt());
		m_pos = DirectX::XMVectorSubtract(quaternion, m_forward);
		DirectX::XMStoreFloat3(&tmpVec3, m_pos);
		m_camera->SetPosition(tmpVec3);
	}
	else if (m_isRightDown) {
		// 鼠标右键按着
		m_pos = DirectX::XMVectorAdd(m_pos,
			DirectX::XMVectorScale(m_left, -delta.x));
		m_pos = DirectX::XMVectorAdd(m_pos,
			DirectX::XMVectorScale(m_up, delta.y));
		// update! position!
		DirectX::XMStoreFloat3(&tmpVec3, m_pos);
		m_camera->SetPosition(tmpVec3);
		// update! lookAt!
		DirectX::XMStoreFloat3(&tmpVec3,
			DirectX::XMVectorAdd(m_pos, m_forward));
		m_camera->SetLookAt(tmpVec3);
	}
}

void OrbitController::MouseEventHandler(MouseButton btn, bool state) {
	switch (btn)
	{
	case MBTN_LEFT:
		m_isLeftDown = state;
		break;
	case MBTN_RIGHT:
		m_isRightDown = state;
		break;
	default:
		break;
	}
}

void OrbitController::KeyEventHandler(KeyButton btn, bool state) {
	// none!
}

void OrbitController::MouseWheel(float w) {
	static XMFLOAT3 tmpVec3;
	// 向前或后移动摄像机
	m_pos = DirectX::XMVectorAdd(m_pos,
		DirectX::XMVectorScale(DirectX::XMVector3Normalize(m_forward), w * MOUSE_MOVE_SCALE));
	m_forward = DirectX::XMVectorSubtract(
		DirectX::XMLoadFloat3(&m_camera->GetLookAt()),
		m_pos
	);
	// update! position!
	DirectX::XMStoreFloat3(&tmpVec3, m_pos);
	m_camera->SetPosition(tmpVec3);
}
