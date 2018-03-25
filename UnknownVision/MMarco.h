#pragma once
class DXRenderer;
class MainClass;
class Material;
class Pipeline;
// 默认摄像机控制器的设置
#define CameraControllerSetting(x) \
	MainClass::UserFunc = [&x](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->void { \
		switch (uMsg) { \
			case WM_MOUSEMOVE: \
				x.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); \
				break; \
			case WM_LBUTTONDOWN: \
				x.MouseEventHandler(MBTN_LEFT, true); \
				break; \
			case WM_LBUTTONUP: \
				x.MouseEventHandler(MBTN_LEFT, false); \
				break; \
			case WM_RBUTTONDOWN: \
				x.MouseEventHandler(MBTN_RIGHT, true); \
				break; \
			case WM_RBUTTONUP: \
				x.MouseEventHandler(MBTN_RIGHT, false); \
				break; \
			case WM_MOUSEWHEEL: \
				x.MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)); \
				break; \
		} \
	};
