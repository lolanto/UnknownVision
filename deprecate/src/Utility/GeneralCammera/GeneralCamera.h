#pragma once
#include <memory>
#include <functional>
#include <array>

namespace UVCameraUtility {
	using MFLOAT2 = std::array<float, 2>;
	using MFLOAT3 = std::array<float, 3>;
	using MFLOAT4X4 = std::array<float, 16>;

	const MFLOAT3 DEFAULT_UP_DIR = { 0.0f, 1.0f, 0.0f }; /**< 默认的“上”方向永远是y轴方向 */

	class CameraController;

	enum MouseButton : uint8_t {
		MBTN_LEFT = 0,
		MBTN_RIGHT,
		MBTN_MID
	};

	enum KeyButton : uint8_t {
		KBTN_W = 0,
		KBTN_A,
		KBTN_S,
		KBTN_D,
		KBTN_SHIFT
	};

	// 摄像机参数说明
	struct CAMERA_DESC {
		MFLOAT3 position;
		MFLOAT3 lookAt;
		// 视场宽度单位是弧度制
		float												fov;
		// 视场宽高
		float												width;
		float												height;
		// 近平面
		float												nearPlane;
		// 远平面
		float												farPlane;
		CAMERA_DESC(float width_, float height_,
			const MFLOAT3& position_,
			const MFLOAT3& lookAt_,
			float fov_ = 1.39f, float nearPlane_ = 0.01f, float farPlane_ = 50.0f)
			: position(position_), lookAt(lookAt_), fov(fov_), height(height_), width(width_), nearPlane(nearPlane_), farPlane(farPlane_) {}
	};

	// 摄像机基础数据
	// 该类应该尽量避免在声明过程中引入任何库相关依赖项，只提供纯粹的接口
	class GeneralCamera {
	public:
		GeneralCamera(CAMERA_DESC&);

	public:
		/** 返回当前摄像机的空间位置
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float */
		void GetPosition(float* output) const;
		/** 返回当前摄像机的“前进方向”
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float，保证输出结果已经标准化*/
		void GetForwardDirection(float* output) const;
		/** 返回当前摄像机的“上方向”
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float，保证输出结果已经标准化*/
		void GetUpDirection(float* output) const;
		/** 返回当前摄像机的“右边方向"
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float，保证输出结果已经标准化*/
		void GetRightDirection(float* output) const;

		/** 返回当前摄像机的view矩阵
		 * @param output 指向存储输出结果的存储空间，必须保证至少能够容纳16个float
		 * @remark 输出的内容与具体实现有关，详见cpp实现文件 */
		void GetViewMat(float* output);
		/** 返回当前摄像机的投影矩阵
		 * @param output 指向存储输出结果的存储空间，必须保证至少能够容纳16个float
		 * @remark 输出的内容与具体实现有关，详见cpp实现文件 */
		void GetProMat(float* output);

		void GetLastFrameViewMat(float* output) const;
		void GetLastFrameProjMat(float* output) const;

		float GetFOV() const;
		float GetASPECT() const;
		float GetNear() const;
		float GetFar() const;

		void SetPosition(float* input);
		void SetForward(float* input);
		void SetFOV(float);

		void SetWidth(float);
		void SetHeight(float);
		/** 每帧处理完之后再执行 */
		void UpdatePerFrameEnd();
	private:
		// 影响到view矩阵的参数发生更改
		// 参数包括：摄像机位置，观察方向
		bool	m_isViewDirty;
		// 影响到projectiong矩阵的参数发生更改
		// 参数包括：fov，长宽
		bool m_isProjDirty;

		float	m_width;
		float	m_height;
		float	m_fov;
		float	m_near;
		float	m_far;

		MFLOAT3 m_position;
		MFLOAT3 m_forward;

		MFLOAT4X4 m_viewMat, m_last_viewMat;
		MFLOAT4X4 m_projMat, m_last_projMat;
	};

	constexpr float MOUSE_MOVE_SCALE = 0.005f;

	// 摄像机控制器接口
	class CameraController {
	public:
		CameraController(GeneralCamera& camera) : m_camera(camera) {}
		virtual void MouseWheel(float) {}
		// 输入最新鼠标屏幕位置
		virtual void MouseMove(float x, float y) = 0;
		// 处理鼠标点击事件
		virtual void MouseEventHandler(MouseButton btn, bool state) = 0;
		// 处理键盘事件
		virtual void KeyEventHandler(KeyButton btn, bool state) = 0;
	protected:
		GeneralCamera& m_camera;
	};

	class EpicCameraController : public CameraController {
	public:
		EpicCameraController(GeneralCamera&);
	public:
		void MouseMove(float x, float y);
		void KeyEventHandler(KeyButton btn, bool state);
		void MouseEventHandler(MouseButton btn, bool state);
	private:
		MFLOAT3 m_right;
		MFLOAT3 m_up;
		MFLOAT3 m_forward;
		MFLOAT3 m_pos;

		MFLOAT2 m_mouseLastPos;
		bool m_isLeftDown;
		bool m_isRightDown;
		bool m_isMidDown;
		bool m_isShiftDown;
	};

} // UVUTILITY