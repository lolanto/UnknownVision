#include "MainClass.h"

MainClass::MainClass(const char* name) {
	m_windowClassName = name;
	m_hInstance = NULL;
}

MainClass::~MainClass() {}

HRESULT MainClass::CreateDesktopWindow(float width, float height) {

	// 获取当前进程实例的句柄
	if (m_hInstance == NULL)
		m_hInstance = (HINSTANCE)GetModuleHandle(NULL);

	WNDCLASS wndClass;
	wndClass.style = CS_DBLCLKS;
	wndClass.lpfnWndProc = MainClass::StaticWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = m_hInstance;
	wndClass.hIcon = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = m_windowClassName.c_str();

	if (!RegisterClass(&wndClass)) {
		DWORD dwError = GetLastError();
		if (dwError != ERROR_CLASS_ALREADY_EXISTS)
			return HRESULT_FROM_WIN32(dwError);
	}


	m_hWnd = CreateWindow(
		m_windowClassName.c_str(),
		m_windowClassName.c_str(),
		WS_OVERLAPPEDWINDOW,
		0, 0,
		width, height,
		0,
		0,
		m_hInstance,
		0
	);

	if (m_hWnd == NULL) {
		DWORD dwError = GetLastError();
		return HRESULT_FROM_WIN32(dwError);
	}

	return S_OK;
}

// 赋予空函数，以免在不设置时出错
std::function<void(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)>
	MainClass::UserFunc = [](HWND, UINT, WPARAM, LPARAM) {};

LRESULT CALLBACK MainClass::StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		UnregisterClass(m_windowClassName.c_str(), m_hInstance);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEWHEEL:
		UserFunc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
