#include "GeneralCamera.h"

#ifdef OUTPUT_MESSAGE
#include <InfoLog/InfoLog.h>
#define MERROR(fmt, ...) LOG_ERROR(fmt, __VA_ARGS__)
#else
#define MERROR(fmt, ...)
#endif // OUTPUT_MESSAGE
#include <string>
#include <unordered_map>
#include <functional>

using namespace UVCameraUtility;

ICamera::ICamera(const CAMERA_DESC& desc)
	: m_fov(desc.fov), m_width(desc.width), m_height(desc.height),
	m_near(desc.nearPlane), m_far(desc.farPlane),
	m_isViewDirty(true), m_isProjDirty(true) {
}

class Camera : public ICamera {
private:
	enum { HISTORY_BUFFER = 2 };
public:
	Camera(const CAMERA_DESC& desc) : ICamera(desc) {
		m_position = desc.position;
		auto lookAt = desc.lookAt;
		m_forward = glm::normalize(lookAt - m_position);
		/** 初始化所有的矩阵数据 */
		m_prevFrame = 0;
		m_curFrame = 0;
		m_isPrevProjDirty = true;
		m_isPrevViewDirty = true;
		{
			glm::mat4x4 projMat = glm::perspectiveFovLH_ZO(m_fov, m_width, m_height, m_near, m_far);
			for (auto& mat : m_projMats) mat = projMat;
		}
		{
			glm::mat4x4 viewMat = glm::lookAtLH(m_position, lookAt, DEFAULT_UP_DIR);
			for (auto& mat : m_viewMats) mat = viewMat;
		}
		{
			for (auto& offset : m_offsets) offset = { 0, 0 };
		}
	}
	virtual ~Camera() = default;
public:
	virtual glm::vec3 GetPosition() const override final {
		return m_position;
	}
	virtual glm::vec3 GetForwardDirection() const override final {
		return m_forward;
	}
	virtual glm::vec3 GetUpDirection() const override final {
		auto rightVec = glm::cross(DEFAULT_UP_DIR, m_forward);
		auto upVec = glm::cross(m_forward, rightVec);
		return upVec;
	}
	virtual glm::vec3 GetRightDirection() const override final {
		auto rightVec = glm::cross(DEFAULT_UP_DIR, m_forward);
		return rightVec;
	}

	virtual GeneralCameraDataStructure GetCameraData() override final {
		GeneralCameraDataStructure ans;
		auto&& pos = GetPosition();
		ans.position = { pos.x, pos.y, pos.z, 1.0f };
		ans.projMat = GetProMat();
		ans.viewMat = GetViewMat();
		return ans;
	}

	virtual glm::mat4x4 GetLastFrameViewMat() const override final {
		return m_viewMats[m_prevFrame];
	}
	virtual glm::mat4x4 GetLastFrameProjMat() const override final {
		return m_projMats[m_prevFrame];
	}
	virtual void UpdatePerFrameEnd() override final {
		int nextFrame = (m_curFrame + 1) % HISTORY_BUFFER;
		m_viewMats[nextFrame] = m_viewMats[m_curFrame];
		m_projMats[nextFrame] = m_projMats[m_curFrame];
		m_prevFrame = m_curFrame;
		m_curFrame = nextFrame;
	}
	virtual void SetPosition(glm::vec3 input) override final {
		m_position = input;
		m_isViewDirty = true;
	}
	virtual void SetForward(glm::vec3 input) override final {
		m_forward = input;	
		m_isViewDirty = true;
	}
	void SetOffset(glm::vec2 offset) override final {
		m_offsets[m_curFrame] = offset;
		m_isProjDirty = true;
	}
	glm::vec2 GetOffset() const override final {
		return m_offsets[m_curFrame];
	}

	glm::mat4x4 GetViewMat() {
		if (m_isViewDirty) {
			auto focus = m_position + m_forward;
			m_viewMats[m_curFrame] = glm::lookAt(m_position, focus, DEFAULT_UP_DIR);
			m_isViewDirty = false;
		}
		return m_viewMats[m_curFrame];
	}

	glm::mat4x4 GetProMat() {
		if (m_isProjDirty) {
			m_projMats[m_curFrame] = glm::perspectiveFov(m_fov, m_width, m_height, m_near, m_far);
			m_projMats[m_curFrame][2][0] = m_offsets[m_curFrame].x * 2 / m_width;
			m_projMats[m_curFrame][2][1] = m_offsets[m_curFrame].y * 2 / m_height;
			m_isProjDirty = false;
		}
		return m_projMats[m_curFrame];
	}
private:
	glm::vec3 m_position; // float3
	glm::vec3 m_forward; // float3
	glm::mat4x4 m_projMats[HISTORY_BUFFER]; // float4x4[2]
	glm::mat4x4 m_viewMats[HISTORY_BUFFER]; // float4x4[2]
	glm::vec2 m_offsets[HISTORY_BUFFER]; // float2 offsetX and offsetY
	int m_curFrame, m_prevFrame;
	mutable bool m_isPrevProjDirty, m_isPrevViewDirty;
};

class EpicCameraController : public ICameraController {
public:
	const float MovingSpeed = 0.5f / 60.0f;
public:
	EpicCameraController(ICamera& camera) : ICameraController(camera),
		m_camera(reinterpret_cast<Camera&>(camera)) {
		m_right = m_camera.GetRightDirection();
		m_up = m_camera.GetUpDirection();
		m_forward = m_camera.GetForwardDirection();
		m_pos = m_camera.GetPosition();
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
			m_pos = m_pos + m_forward * offset * MovingSpeed * deltaTime;
			m_camera.SetPosition(m_pos);
		}
		if (m_isWASDDown[1] || m_isWASDDown[3]) {
			float offset = m_isWASDDown[1] ? -1.0f : 1.0f;
			m_pos = m_pos + m_right * offset * MovingSpeed * deltaTime;
			m_camera.SetPosition(m_pos);
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
		glm::vec2 deltaRadiance = { (x - m_mouseLastPos.x) * MOUSE_MOVE_SCALE, (y - m_mouseLastPos.y) * MOUSE_MOVE_SCALE };
		glm::vec2 deltaDistance = { (x - m_mouseLastPos.x) / 100.0f, (y - m_mouseLastPos.y) / 100.0f };
		m_mouseLastPos = { x, y };
		if (m_isLeftDown) {
			/** 鼠标左键按着	 摄像机前进后退
			 * 往上是摄像机朝forward投影到xz平面方向移动，往后则后移
			 * 向左是朝左边旋转，绕y轴逆时针旋转，向右是绕y轴顺时针旋转 */
			m_forward = glm::rotate(m_forward, deltaRadiance.x, DEFAULT_UP_DIR);
			m_right = glm::rotate(m_right, deltaRadiance.x, DEFAULT_UP_DIR);
			m_up = glm::rotate(m_up, deltaDistance.x, DEFAULT_UP_DIR);

			auto dir = glm::cross(m_right, DEFAULT_UP_DIR) * -deltaDistance.y;
			m_pos = m_pos + dir;

			m_camera.SetForward(m_forward);
			m_camera.SetPosition(m_pos);
		}
		else if (m_isRightDown) {
			/** 鼠标右键控制摄像机的朝向，在原地不动，旋转摄像机
			 * 向上是抬头，绕right轴逆时针旋转，向下是低头，绕right轴顺时针旋转
			 * 向左是朝左边旋转，绕y轴逆时针旋转，向右是绕y轴顺时针旋转 */
			m_forward = glm::rotate(m_forward, deltaRadiance.x, DEFAULT_UP_DIR);
			m_right = glm::rotate(m_right, deltaRadiance.x, DEFAULT_UP_DIR);
			m_up = glm::rotate(m_up, deltaRadiance.x, DEFAULT_UP_DIR);
			m_forward = glm::rotate(m_forward, deltaRadiance.y, m_right);
			m_up = glm::rotate(m_up, deltaRadiance.y, m_right);

			m_camera.SetForward(m_forward);
		}
		else if (m_isMidDown) {
			/** 鼠标中键控制摄像机在进行上下左右平移
			 * 上下是沿着y轴移动
			 * 左右是沿着自己的right轴移动 */
			m_pos = m_pos - DEFAULT_UP_DIR * deltaDistance.y;
			m_pos = m_pos + m_right * deltaDistance.x;
			m_camera.SetPosition(m_pos);
		}
	}
private:
	glm::vec3 m_right;
	glm::vec3 m_up;
	glm::vec3 m_pos;
	glm::vec3 m_forward;
	glm::vec2 m_mouseLastPos;
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


