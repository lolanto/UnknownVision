#pragma once
#include <memory>
#include <functional>
#include <array>
#include <MathInterface/MathInterface.hpp>
#define USING_DXMATH
#define OUTPUT_MESSAGE /**< 输出错误信息，定义这个宏将会导致代码依赖Utility/InfoLog/InfoLog.h */
namespace UVCameraUtility {

	const  IMath::IFLOAT3 DEFAULT_UP_DIR = { 0.0f, 1.0f, 0.0f }; /**< 默认的“上”方向永远是y轴方向 */

	class ICameraController;
	class ICmaera;

	enum MouseButton : uint8_t {
		MOUSE_BUTTON_NONE,
		MOUSE_BUTTON_LEFT,
		MOUSE_BUTTON_RIGHT,
		MOUSE_BUTTON_MID
	};

	enum KeyButton : uint8_t {
		KEY_BUTTON_NONE,
		KEY_BUTTON_W,
		KEY_BUTTON_A,
		KEY_BUTTON_S,
		KEY_BUTTON_D,
		KEY_BUTTON_SHIFT
	};

	enum ControllerType {
		CONTROLLER_TYPE_EPIC
	};

	/** 摄像机数据缓冲的内存结构
	 * Note: 该缓冲的结构必须与代码文件GeneralCamera.hlsl.inc中的同名结构题 **完全一致** */
	struct GeneralCameraDataStructure {
		IMath::IFLOAT4 position;
		IMath::IFLOAT4X4 viewMat;
		IMath::IFLOAT4X4 projMat;
	};

	// 摄像机参数说明
	struct CAMERA_DESC {
		IMath::IFLOAT3 position;
		IMath::IFLOAT3 lookAt;
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
			const IMath::IFLOAT3& position_,
			const IMath::IFLOAT3& lookAt_,
			float fov_ = 1.39f, float nearPlane_ = 0.01f, float farPlane_ = 50.0f)
			: position(position_), lookAt(lookAt_), fov(fov_), height(height_), width(width_), nearPlane(nearPlane_), farPlane(farPlane_) {}
		CAMERA_DESC() = default;
	};

	// 摄像机基础数据
	// 该类应该尽量避免在声明过程中引入任何库相关依赖项，只提供纯粹的接口
	class ICamera {
	public:
		ICamera(const CAMERA_DESC&);
		virtual ~ICamera() = default;
	public:
		/** 返回当前摄像机的空间位置
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float */
		virtual IMath::IFLOAT3 GetPosition() const = 0;
		/** 返回当前摄像机的“前进方向”
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float，保证输出结果已经标准化*/
		virtual IMath::IFLOAT3 GetForwardDirection() const = 0;
		/** 返回当前摄像机的“上方向”
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float，保证输出结果已经标准化*/
		virtual IMath::IFLOAT3 GetUpDirection() const = 0;
		/** 返回当前摄像机的“右边方向"
		 * @param output 指向存储输出结果的存储空间，必须至少能够容纳3个float，保证输出结果已经标准化*/
		virtual IMath::IFLOAT3 GetRightDirection() const = 0;

		/** 返回摄像机缓冲数据，具体实现见CPP文件 */
		virtual GeneralCameraDataStructure GetCameraData() = 0;

		virtual IMath::IFLOAT4X4 GetLastFrameViewMat() const = 0;
		virtual IMath::IFLOAT4X4 GetLastFrameProjMat() const = 0;

		float GetFOV() const { return m_fov; }
		float GetASPECT() const { return m_width / m_height; }
		float GetNear() const { return m_near; }
		float GetFar() const { return m_far; }

		virtual void SetPosition(IMath::IFLOAT3 input) = 0;
		virtual void SetForward(IMath::IFLOAT3 input) = 0;
		void SetFOV(float input) { m_fov = input; m_isProjDirty = true; }

		void SetWidth(float input) { m_width = input; m_isProjDirty = true; }
		void SetHeight(float input) { m_height = input; m_isProjDirty = true; }
		/** 每帧处理完之后再执行 */
		virtual void UpdatePerFrameEnd() {};
	protected:
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
	};

	constexpr float MOUSE_MOVE_SCALE = 0.005f;

	// 摄像机控制器接口
	class ICameraController {
	public:
		ICameraController(ICamera& camera) {};
		virtual ~ICameraController() = default;
		virtual void MouseWheel(float) {}
		virtual void KeyCallback(KeyButton key, bool isPressed) = 0;
		virtual void MouseCallback(float x, float y, MouseButton btn, bool isPressed) = 0;
		virtual void CalledPerFrame(float deltaTime) = 0;
	};

	std::pair<ICamera*, ICameraController*> CreateCamera(const CAMERA_DESC& desc, ControllerType type);


} // UVUTILITY
