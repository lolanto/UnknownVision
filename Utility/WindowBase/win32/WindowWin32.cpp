#include "WindowWin32.h"
#include <windowsx.h>
#include <chrono>
using namespace UVWindows;
LRESULT WindowWin32::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	size_t xpos, ypos;
	switch (uMsg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
		xpos = GET_X_LPARAM(lParam);
		ypos = GET_Y_LPARAM(lParam);
		MouseCallBack(xpos, ypos, MOUSE_EVENT_NONE, false);
		return 0;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_SHIFT:
			KeyboardCallBack(KEY_BOARD_EVENT_SHIFT, true);
			break;
		default:
			break;
		}
		return 0;
	case WM_KEYUP:
		switch (wParam) {
		case VK_SHIFT:
			KeyboardCallBack(KEY_BOARD_EVENT_SHIFT, false);
			break;
		default:
			break;
		}
		return 0;
	case WM_LBUTTONDOWN:
		xpos = GET_X_LPARAM(lParam);
		ypos = GET_Y_LPARAM(lParam);
		MouseCallBack(xpos, ypos, MOUSE_EVENT_LEFT_BUTTON, true);
		return 0;
	case WM_LBUTTONUP:
		xpos = GET_X_LPARAM(lParam);
		ypos = GET_Y_LPARAM(lParam);
		MouseCallBack(xpos, ypos, MOUSE_EVENT_LEFT_BUTTON, false);
		return 0;
	case WM_RBUTTONDOWN:
		xpos = GET_X_LPARAM(lParam);
		ypos = GET_Y_LPARAM(lParam);
		MouseCallBack(xpos, ypos, MOUSE_EVENT_RIGHT_BUTTON, true);
		return 0;
	case WM_RBUTTONUP:
		xpos = GET_X_LPARAM(lParam);
		ypos = GET_Y_LPARAM(lParam);
		MouseCallBack(xpos, ypos, MOUSE_EVENT_RIGHT_BUTTON, false);
		return 0;
	case WM_MBUTTONDOWN:
		xpos = GET_X_LPARAM(lParam);
		ypos = GET_Y_LPARAM(lParam);
		MouseCallBack(xpos, ypos, MOUSE_EVENT_MID_BUTTON, true);
		return 0;
	case WM_MBUTTONUP:
		xpos = GET_X_LPARAM(lParam);
		ypos = GET_Y_LPARAM(lParam);
		MouseCallBack(xpos, ypos, MOUSE_EVENT_MID_BUTTON, false);
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

	DWORD wStyle = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX ^ WS_MINIMIZEBOX;

	RECT rect;
	rect.left = 0; rect.top = 0;
	rect.right = m_width; rect.bottom = m_height;
	AdjustWindowRect(&rect, wStyle, false); /**< 计算width, height大小的Client Area实际需要多大的窗口(窗口包括标题栏和工具栏之类的东西) */

	m_hWnd = CreateWindow(
		m_name.c_str(),
		m_name.c_str(),
		wStyle,
		0, 0,
		rect.right - rect.left, rect.bottom - rect.top,
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
	auto start = std::chrono::high_resolution_clock::now();
	auto end = std::chrono::high_resolution_clock::now();
	while (msg.message != WM_QUIT) {
		end = std::chrono::high_resolution_clock::now();
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			std::chrono::duration<float, std::micro> delta = (end - start);
			MainLoop(delta.count() / 1000.0f);
		}
		start = end;
	}
}
