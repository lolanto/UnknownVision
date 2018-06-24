#pragma once
class DXRenderer;
class MainClass;

#define DefaultParameters DXRenderer* renderer, MainClass* mc
#define BindSourceUA(t, sbt, slot)  BindSource(static_cast<IUnorderAccess*>(t),sbt, slot)
#define BindSourceTex(t, sbt, slot) BindSource(static_cast<ITexture*>(t), sbt, slot)
#define BindSourceBuf(t, sbt, slot) BindSource(static_cast<IBuffer*>(t), sbt, slot)

#define ToUA(t) static_cast<IUnorderAccess*>(t)
#define ToTex(t) static_cast<ITexture*>(t)

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

extern const float WIDTH;
extern const float HEIGHT;

void ImageBasedLighting(DefaultParameters);

void ScreenSpaceRayTracing(DefaultParameters);

void LTC(DefaultParameters);

void PullPush(DefaultParameters);

void MyALG(DefaultParameters);

void UITest(DefaultParameters);
