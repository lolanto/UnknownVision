#include "WindowWin32.h"

LRESULT WindowWin32::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEWHEEL:	
		/** TODO: 需要响应鼠标事件 */
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool WindowWin32::Init() {
	// 获取当前进程实例的句柄
	HINSTANCE	hInstance = (HINSTANCE)GetModuleHandle(NULL);

	WNDCLASS wndClass;
	wndClass.style = CS_DBLCLKS;
	wndClass.lpfnWndProc = WindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = m_name.c_str();

	if (!RegisterClass(&wndClass)) {
		DWORD dwError = GetLastError();
		if (dwError != ERROR_CLASS_ALREADY_EXISTS)
			return false;
	}


	m_hWnd = CreateWindow(
		m_name.c_str(),
		m_name.c_str(),
		WS_OVERLAPPEDWINDOW,
		0, 0,
		m_width, m_height,
		0,
		0,
		hInstance,
		0
	);

	if (m_hWnd == nullptr) {
		return false;
	}
	return true;
}

void WindowWin32::Run() {
	MSG msg;
	msg.message = NULL;

	if (!IsWindowVisible(m_hWnd)) ShowWindow(m_hWnd, SW_SHOW);

	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			MainLoop(0.042f);
		}
	}
}
