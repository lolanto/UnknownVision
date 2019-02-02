#ifndef WINDOW_BASE_H
#define WINDOW_BASE_H
#include <string>
#include <functional>
using MainLoopFunctionPointer = std::function<void(float deltaTIme)>;
using MouseEventCallBack = std::function<void(float x, float y, float dx, float dy, float deltaTime)>;
using KeyboardEventCallBack = std::function<void(uint32_t keyAscii, bool isPressed, float deltaTime)>;
class WindowBase {
public:
	static MainLoopFunctionPointer && MainLoop; /**< 窗口运行时的主循环 */
	static MouseEventCallBack&& MouseCallBack; /**< 窗口鼠标事件的回调函数 */
	static KeyboardEventCallBack&& KeyboardCallBack; /**< 窗口键盘事件的回调函数 */
public:
	/// 窗口基类构造函数
	/** 只是存储基本，通用的窗口属性
	 * @param name 窗口名称
	 * @param width 窗口宽度
	 * @param height 窗口高度
	 * @param windowed 窗口是否为窗口化
	 */
	WindowBase(const char* name, float width, float height, bool windowed) 
		: m_name(name), m_width(width), m_height(height),m_isWindowed(windowed) {}
	/** 虚析构函数 */
	virtual ~WindowBase() {}
public:
	/** 窗口初始化函数，由具体实现提供定义 
	 * @return 初始化成功返回true，失败返回false
	 */
	virtual bool Init() = 0;
	/** 窗口运行函数，由具体实现提供定义 */
	virtual void Run() = 0;
	void SetMainLoopFuncPtr(MainLoopFunctionPointer&& funcPtr) { MainLoop = funcPtr; }
	void SetMouseEventFuncPtr(MouseEventCallBack&& funcPtr) { MouseCallBack = funcPtr; }
	void SetKeyboardEventFuncPtr(KeyboardEventCallBack&& funcPtr) { KeyboardCallBack = funcPtr; }
protected:
	float m_width, m_height;
	std::string m_name;
	bool m_isWindowed;

};

#endif // WINDOW_BASE_H
