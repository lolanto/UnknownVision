#pragma once

#include <Windows.h>
#include <string>
#include <functional>

class MainClass {
public:
	MainClass(const char* name = "MainWindow");
	~MainClass();
	// 创建窗口
	HRESULT CreateDesktopWindow( float width, float height );
	// 窗口每一帧运行逻辑
	// f: 每一帧运行的函数
	template<typename func>
	HRESULT Run(func f) {
		MSG msg;
		msg.message = NULL;

		if (!IsWindowVisible(m_hWnd)) ShowWindow(m_hWnd, SW_SHOW);

		while (msg.message != WM_QUIT) {
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				f();
			}
		}

		return S_OK;
	}
	// 获取窗口句柄
	HWND GetWindowHandle() { return m_hWnd; }
	// 窗口事件处理函数
	static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static std::function<void(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)> UserFunc;
private:
	HWND m_hWnd;
};

static HINSTANCE m_hInstance;
static std::string m_windowClassName;
