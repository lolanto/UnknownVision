#ifndef WINDOW_WIN32_H
#define WINDOW_WIN32_H
#include "../WindowBase.h"
#include <Windows.h>
class WindowWin32 : public WindowBase {
public:
	/// WIN32窗口事件处理函数
	/** Win32窗口事件处理函数，需要在遇到规定的事件时触发对应事件的回调函数
	 * @remark 该函数由系统调用
	 */
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	/// WIN32窗口构造函数
	/** 只是存储基本，通用的窗口属性
	* @param name 窗口名称
	* @param width 窗口宽度
	* @param height 窗口高度
	* @param windowed 窗口是否为窗口化
	*/
	WindowWin32(const char* name, uint32_t width, uint32_t height, bool windowed)
		: WindowBase(name, width, height, windowed) {}
	virtual ~WindowWin32() {}
public:
	/** 窗口初始化函数，结合WinAPI实现 */
	bool Init();
	/** 窗口运行函数，负责进行窗口事件分发 */
	void Run();
	/** 返回窗口句柄 */
	const HWND& hWnd() const { return m_hWnd; }
private:
	HINSTANCE m_hInstance = nullptr; /**< 该进程的进程句柄，由WINAPI获得 */
	HWND m_hWnd = nullptr; /**< 申请的窗口句柄，由WINAPI获得 */
};

#endif // WINDOW_WIN32_H
