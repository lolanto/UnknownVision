#include "GeneralCamera.h"

#ifdef OUTPUT_MESSAGE
#include <../Utility/InfoLog/InfoLog.h>
#define MERROR(fmt, ...) LOG_ERROR(fmt, __VA_ARGS__)
#else
#define MERROR(fmt, ...)
#endif // OUTPUT_MESSAGE
#include <string>
#include <unordered_map>
#include <functional>

using namespace UVCameraUtility;
using namespace IMath;

ICamera::ICamera(const CAMERA_DESC& desc)
	: m_fov(desc.fov), m_width(desc.width), m_height(desc.height),
	m_near(desc.nearPlane), m_far(desc.farPlane),
	m_isViewDirty(true), m_isProjDirty(true) {
}

#ifdef USING_DXMATH
#include <DirectXMath.h>

class Camera : public ICamera {
private:
	enum { HISTORY_BUFFER = 2 };
public:
	Camera(const CAMERA_DESC& desc) : ICamera(desc) {
		m_position = DirectX::XMVectorSet(desc.position.x, desc.position.y, desc.position.z, 1.0f);
		auto lookAt = DirectX::XMVectorSet(desc.lookAt.x, desc.lookAt.y, desc.lookAt.z, 1.0f);
		m_forward = DirectX::XMVector3Normalize(
			DirectX::XMVectorSubtract(lookAt, m_position));
		/** 初始化所有的矩阵数据 */
		m_prevFrame = 0;
		m_curFrame = 0;
		m_isPrevProjDirty = true;
		m_isPrevViewDirty = true;
		{
			DirectX::XMMATRIX projMat = DirectX::XMMatrixPerspectiveFovLH(m_fov, m_width / m_height, m_near, m_far);
			for (auto& mat : m_projMats) mat = projMat;
		}
		{

			DirectX::XMMATRIX viewMat = DirectX::XMMatrixLookAtLH(m_position, lookAt,
				DirectX::XMVectorSet(DEFAULT_UP_DIR.x, DEFAULT_UP_DIR.y, DEFAULT_UP_DIR.z, 0.0f)
				);
			for (auto& mat : m_viewMats) mat = viewMat;
		}
	}
	virtual ~Camera() = default;
public:
	virtual IFLOAT3 GetPosition() const override final {
		DirectX::XMFLOAT3 pos;
		DirectX::XMStoreFloat3(&pos, m_position);
		return { pos.x, pos.y, pos.z };
	}
	virtual IFLOAT3 GetForwardDirection() const override final {
		DirectX::XMFLOAT3 dir;
		DirectX::XMStoreFloat3(&dir, m_forward);
		return { dir.x, dir.y, dir.z };
	}
	virtual IFLOAT3 GetUpDirection() const override final {
		auto rightVec = DirectX::XMVector3Cross(
			DirectX::XMVectorSet(DEFAULT_UP_DIR.x, DEFAULT_UP_DIR.y, DEFAULT_UP_DIR.z, 1.0f),
			m_forward
		);
		auto upVec = DirectX::XMVector3Cross( m_forward, rightVec );
		DirectX::XMFLOAT3 up;
		DirectX::XMStoreFloat3(&up, upVec);
		return { up.x, up.y, up.z };
	}
	virtual IFLOAT3 GetRightDirection() const override final {
		auto rightVec = DirectX::XMVector3Cross(
			DirectX::XMVectorSet(DEFAULT_UP_DIR.x, DEFAULT_UP_DIR.y, DEFAULT_UP_DIR.z, 1.0f),
			m_forward
		);
		DirectX::XMFLOAT3 right;
		DirectX::XMStoreFloat3(&right, rightVec);
		return { right.x, right.y, right.z };
	}

	virtual GeneralCameraDataStructure GetCameraData() override final {
		GeneralCameraDataStructure ans;
		auto&& pos = GetPosition();
		ans.position = { pos.x, pos.y, pos.z, 1.0f };
		ans.projMat = GetProMat();
		ans.viewMat = GetViewMat();
		return ans;
	}

	virtual IFLOAT4X4 GetLastFrameViewMat() const override final {
		static IFLOAT4X4 ret;
		if (m_isPrevViewDirty) {
			DirectX::XMFLOAT4X4 viewMat;
			DirectX::XMStoreFloat4x4(&viewMat, m_viewMats[m_prevFrame]);
			ret = {
			viewMat._11, viewMat._12, viewMat._13, viewMat._14,
			viewMat._21, viewMat._22, viewMat._23, viewMat._24,
			viewMat._31, viewMat._32, viewMat._33, viewMat._34,
			viewMat._41, viewMat._42, viewMat._43, viewMat._44
			};
			m_isPrevViewDirty = false;
		}
		return ret;
	}
	virtual IFLOAT4X4 GetLastFrameProjMat() const override final {
		static IFLOAT4X4 ret;
		if (m_isPrevProjDirty) {
			DirectX::XMFLOAT4X4 projMat;
			DirectX::XMStoreFloat4x4(&projMat, m_projMats[m_prevFrame]);
			ret = {
				projMat._11, projMat._12, projMat._13, projMat._14,
				projMat._21, projMat._22, projMat._23, projMat._24,
				projMat._31, projMat._32, projMat._33, projMat._34,
				projMat._41, projMat._42, projMat._43, projMat._44
			};
			m_isPrevProjDirty = false;
		}
		return ret;
	}
	virtual void UpdatePerFrameEnd() override final {
		/** 每一帧结束的时候就将当前帧的view, proj数据和上一帧的进行比对
		 * 假如相同，证明上下两帧没有差异，假如有就设置dirty变量
		 * 同时将当前帧的数据覆盖下一帧的数据内容 */
		int nextFrame = (m_curFrame + 1) % HISTORY_BUFFER;
		{
			DirectX::XMFLOAT4X4 prev, next;
			DirectX::XMStoreFloat4x4(&prev, m_projMats[m_prevFrame]);
			DirectX::XMStoreFloat4x4(&next, m_projMats[m_curFrame]);
			m_isPrevProjDirty = (memcmp(&prev, &next, sizeof(DirectX::XMFLOAT4X4)) == 0);
			DirectX::XMStoreFloat4x4(&prev, m_viewMats[m_prevFrame]);
			DirectX::XMStoreFloat4x4(&next, m_viewMats[m_curFrame]);
			m_isPrevViewDirty = (memcmp(&prev, &next, sizeof(DirectX::XMFLOAT4X4)) == 0);
		}
		/** 假如下一帧没有进行任何操作(即dirty位全都为false)，同时这里没有进行任何拷贝
		 * 就有可能造成当前帧数据没有覆盖历史帧数据，后续访问历史帧数据就会出错 */
		m_projMats[nextFrame] = m_projMats[m_curFrame];
		m_viewMats[nextFrame] = m_viewMats[m_curFrame];
		m_prevFrame = m_curFrame;
		m_curFrame = nextFrame;
	}
	virtual void SetPosition(IFLOAT3 input) override final {
		m_position = DirectX::XMVectorSet(input.x, input.y, input.z, 0.0f);
		m_isViewDirty = true;
	}
	virtual void SetForward(IFLOAT3 input) override final {
		m_forward = DirectX::XMVectorSet(input.x, input.y, input.z, 0.0f);
		m_isViewDirty = true;
	}
public: /** 与数学库相关的，仅子类拥有的实现 */
	DirectX::XMVECTOR _GetUpDirection() const { 
		auto&& rightVec = _GetRightDirection();
		auto upVec = DirectX::XMVector3Cross(m_forward, rightVec);
		return upVec;
	}
	DirectX::XMVECTOR _GetRightDirection() const {
		return DirectX::XMVector3Cross(
			DirectX::XMVectorSet(DEFAULT_UP_DIR.x, DEFAULT_UP_DIR.y, DEFAULT_UP_DIR.z, 1.0f),
			m_forward
		);
	}
	DirectX::XMVECTOR _GetForwardDirection() const { return m_forward; }
	DirectX::XMVECTOR _GetPosition() const { return m_position; }
	void _SetPosition(DirectX::XMVECTOR input) { m_position = input; m_isViewDirty = true; }
	void _SetForward(DirectX::XMVECTOR input) { m_forward = input; m_isViewDirty = true; }

	IFLOAT4X4 GetViewMat() {
		static IFLOAT4X4 ret;
		if (m_isViewDirty) {
			DirectX::XMVECTOR focus = DirectX::XMVectorAdd(m_position, m_forward);
			m_viewMats[m_curFrame] = DirectX::XMMatrixLookAtLH(
				m_position,
				focus,
				DirectX::XMVectorSet(DEFAULT_UP_DIR.x, DEFAULT_UP_DIR.y, DEFAULT_UP_DIR.z, 0.0f)
			);
			DirectX::XMFLOAT4X4 viewMat;
			DirectX::XMStoreFloat4x4(&viewMat, m_viewMats[m_curFrame]);
			ret = {
			viewMat._11, viewMat._12, viewMat._13, viewMat._14,
			viewMat._21, viewMat._22, viewMat._23, viewMat._24,
			viewMat._31, viewMat._32, viewMat._33, viewMat._34,
			viewMat._41, viewMat._42, viewMat._43, viewMat._44
			};
			m_isViewDirty = false;
		}

		return ret;
	}

	IFLOAT4X4 GetProMat() {
		static IFLOAT4X4 ret;
		if (m_isProjDirty) {
			m_projMats[m_curFrame] = DirectX::XMMatrixPerspectiveFovLH(m_fov, m_width / m_height, m_near, m_far);
			DirectX::XMFLOAT4X4 projMat;
			DirectX::XMStoreFloat4x4(&projMat, m_projMats[m_curFrame]);
			ret = {
				projMat._11, projMat._12, projMat._13, projMat._14,
				projMat._21, projMat._22, projMat._23, projMat._24,
				projMat._31, projMat._32, projMat._33, projMat._34,
				projMat._41, projMat._42, projMat._43, projMat._44
			};
			m_isProjDirty = false;
		}
		return ret;
	}
private:
	DirectX::XMVECTOR m_position; // float3
	DirectX::XMVECTOR m_forward; // float3
	DirectX::XMMATRIX m_projMats[HISTORY_BUFFER]; // float4x4[2]
	DirectX::XMMATRIX m_viewMats[HISTORY_BUFFER]; // float4x4[2]
	int m_curFrame, m_prevFrame;
	mutable bool m_isPrevProjDirty, m_isPrevViewDirty;
};

class EpicCameraController : public ICameraController {
public:
	const float MovingSpeed = 0.5f / 60.0f;
public:
	EpicCameraController(ICamera& camera) : ICameraController(camera),
		m_camera(reinterpret_cast<Camera&>(camera)) {
		m_right = m_camera._GetRightDirection();
		m_up = m_camera._GetUpDirection();
		m_forward = m_camera._GetForwardDirection();
		m_pos = m_camera._GetPosition();
		m_mouseLastPos = { FLT_MAX, FLT_MAX };
		m_isLeftDown = false;
		m_isRightDown = false;
		m_isMidDown = false;
		m_isShiftDown = false;
		m_isWASDDown[0] = false; m_isWASDDown[1] = false; m_isWASDDown[2] = false; m_isWASDDown[3] = false;
	}
	virtual ~EpicCameraController() = default;
	virtual void KeyCallback(KeyButton key, bool isPressed) override final {
		if (key == KEY_BUTTON_SHIFT) m_isShiftDown = isPressed;
		if (key == KEY_BUTTON_W) m_isWASDDown[0] = isPressed;
		if (key == KEY_BUTTON_A) m_isWASDDown[1] = isPressed;
		if (key == KEY_BUTTON_S) m_isWASDDown[2] = isPressed;
		if (key == KEY_BUTTON_D) m_isWASDDown[3] = isPressed;
	}
	virtual void CalledPerFrame(float deltaTime) override final {
		if (m_isWASDDown[0] || m_isWASDDown[2]) {
			float offset = m_isWASDDown[0] ? 1.0f : -1.0f;
			DirectX::XMVECTOR dir = DirectX::XMVectorScale(m_forward, offset * MovingSpeed * deltaTime);
			m_pos = DirectX::XMVectorAdd(m_pos, dir);
			m_camera._SetPosition(m_pos);
		}
		if (m_isWASDDown[1] || m_isWASDDown[3]) {
			float offset = m_isWASDDown[1] ? -1.0f : 1.0f;
			DirectX::XMVECTOR dir = DirectX::XMVectorScale(m_right, offset * MovingSpeed * deltaTime);
			m_pos = DirectX::XMVectorAdd(m_pos, dir);
			m_camera._SetPosition(m_pos);
		}
	}
	virtual void MouseCallback(float x, float y, MouseButton btn, bool isPressed) override final {
		switch (btn) {
		case MOUSE_BUTTON_LEFT:
			m_isLeftDown = isPressed;
			break;
		case MOUSE_BUTTON_RIGHT:
			m_isRightDown = isPressed;
			break;
		case MOUSE_BUTTON_MID:
			m_isMidDown = isPressed;
			break;
		}
		/** 状态初始化 */
		if (m_mouseLastPos.x == FLT_MAX && m_mouseLastPos.y == FLT_MAX) {
			m_mouseLastPos = { x, y };
			return;
		}
		IFLOAT2 deltaRadiance = { (x - m_mouseLastPos.x) * MOUSE_MOVE_SCALE, (y - m_mouseLastPos.y) * MOUSE_MOVE_SCALE };
		IFLOAT2 deltaDistance = { (x - m_mouseLastPos.x) / 100.0f, (y - m_mouseLastPos.y) / 100.0f };
		m_mouseLastPos = { x, y };
		if (m_isLeftDown) {
			/** 鼠标左键按着	 摄像机前进后退
			 * 往上是摄像机朝forward投影到xz平面方向移动，往后则后移
			 * 向左是朝左边旋转，绕y轴逆时针旋转，向右是绕y轴顺时针旋转 */
			auto defUp = DirectX::XMVectorSet(DEFAULT_UP_DIR.x, DEFAULT_UP_DIR.y, DEFAULT_UP_DIR.z, 0.0f);
			auto&& quaternion = DirectX::XMQuaternionRotationAxis(defUp, deltaRadiance.x);
			m_forward = DirectX::XMVector3Rotate(m_forward, quaternion);
			m_right = DirectX::XMVector3Rotate(m_right, quaternion);
			m_up = DirectX::XMVector3Rotate(m_up, quaternion);


			DirectX::XMVECTOR dir = DirectX::XMVector3Cross(m_right, defUp);
			dir = DirectX::XMVectorScale(dir, -deltaDistance.y);
			m_pos = DirectX::XMVectorAdd(m_pos, dir);

			m_camera._SetForward(m_forward);
			m_camera._SetPosition(m_pos);
		}
		else if (m_isRightDown) {
			/** 鼠标右键控制摄像机的朝向，在原地不动，旋转摄像机
			 * 向上是抬头，绕right轴逆时针旋转，向下是低头，绕right轴顺时针旋转
			 * 向左是朝左边旋转，绕y轴逆时针旋转，向右是绕y轴顺时针旋转 */
			auto defUp = DirectX::XMVectorSet(DEFAULT_UP_DIR.x, DEFAULT_UP_DIR.y, DEFAULT_UP_DIR.z, 0.0f);
			auto&& quaternion = DirectX::XMQuaternionRotationAxis(defUp, deltaRadiance.x);
			m_forward = DirectX::XMVector3Rotate(m_forward, quaternion);
			m_right = DirectX::XMVector3Rotate(m_right, quaternion);
			m_up = DirectX::XMVector3Rotate(m_up, quaternion);
			quaternion = DirectX::XMQuaternionRotationAxis(m_right, deltaRadiance.y);
			m_forward = DirectX::XMVector3Rotate(m_forward, quaternion);
			m_up = DirectX::XMVector3Rotate(m_up, quaternion);

			m_camera._SetForward(m_forward);
		}
		else if (m_isMidDown) {
			/** 鼠标中键控制摄像机在进行上下左右平移
			 * 上下是沿着y轴移动
			 * 左右是沿着自己的right轴移动 */
			m_pos = DirectX::XMVectorAdd(m_pos, DirectX::XMVectorSet(DEFAULT_UP_DIR.x * -deltaDistance.y, DEFAULT_UP_DIR.y * -deltaDistance.y, DEFAULT_UP_DIR.z * -deltaDistance.y, 0.0f));
			m_pos = DirectX::XMVectorAdd(m_pos, DirectX::XMVectorScale(m_right, deltaDistance.x));
			m_camera._SetPosition(m_pos);
		}
	}
private:
	DirectX::XMVECTOR m_right;
	DirectX::XMVECTOR m_up;
	DirectX::XMVECTOR m_pos;
	DirectX::XMVECTOR m_forward;
	IFLOAT2 m_mouseLastPos;
	bool m_isLeftDown;
	bool m_isRightDown;
	bool m_isMidDown;
	bool m_isShiftDown;
	bool m_isWASDDown[4]; /**< 分别对应WASD */
	Camera& m_camera;
};

std::pair<ICamera*, ICameraController*> UVCameraUtility::CreateCamera(const CAMERA_DESC& desc, ControllerType type)
{
	ICamera* pCamera = new Camera(desc);
	ICameraController* pController = nullptr;
	switch (type) {
	case CONTROLLER_TYPE_EPIC:
		pController = new EpicCameraController(*pCamera);
		break;
	default:
		abort();
	}
	return { pCamera, pController };
}
#endif // USING_DXMATH

