﻿#ifndef WINDOW_BASE_H
#define WINDOW_BASE_H
#include <string>
#include <functional>
namespace UVWindows {

	enum MouseEvent : uint8_t {
		MOUSE_EVENT_NONE,
		MOUSE_EVENT_LEFT_BUTTON,
		MOUSE_EVENT_RIGHT_BUTTON,
		MOUSE_EVENT_MID_BUTTON
	};

	enum KeyBoardEvent : uint8_t {
		KEY_BOARD_EVENT_SHIFT
	};

	using MainLoopFunctionPointer = std::function<void(float deltaTIme)>;
	using MouseEventCallBack = std::function<void(float x, float y, MouseEvent eve, bool pressed)>;
	using KeyboardEventCallBack = std::function<void(KeyBoardEvent eve, bool pressed)>;
	class WindowBase {
	public:
		static MainLoopFunctionPointer MainLoop; /**< 窗口运行时的主循环 */
		static MouseEventCallBack MouseCallBack; /**< 窗口鼠标事件的回调函数 */
		static KeyboardEventCallBack KeyboardCallBack; /**< 窗口键盘事件的回调函数 */
	public:
		/// 窗口基类构造函数
		/** 只是存储基本，通用的窗口属性
		 * @param name 窗口名称
		 * @param width 窗口宽度
		 * @param height 窗口高度
		 * @param windowed 窗口是否为窗口化
		 */
		WindowBase(const char* name, uint32_t width, uint32_t height, bool windowed)
			: m_name(name), m_width(width), m_height(height), m_isWindowed(windowed) {}
		/** 虚析构函数 */
		virtual ~WindowBase() {}
	public:
		/** 窗口初始化函数，由具体实现提供定义
		 * @return 初始化成功返回true，失败返回false
		 */
		virtual bool Init() = 0;
		/** 窗口运行函数，由具体实现提供定义 */
		virtual void Run() = 0;
		uint32_t Width() const { return m_width; }
		uint32_t Height() const { return m_height; }
		void SetMainLoopFuncPtr(MainLoopFunctionPointer&& funcPtr) { MainLoop = funcPtr; }
		void SetMouseEventFuncPtr(MouseEventCallBack&& funcPtr) { MouseCallBack = funcPtr; }
		void SetKeyboardEventFuncPtr(KeyboardEventCallBack&& funcPtr) { KeyboardCallBack = funcPtr; }
	protected:
		uint32_t m_width, m_height;
		std::string m_name;
		bool m_isWindowed;

	};
} // UVWindows
#endif // WINDOW_BASE_H
