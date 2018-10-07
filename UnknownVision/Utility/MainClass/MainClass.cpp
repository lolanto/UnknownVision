#include "MainClass.h"
#include "InfoLog.h"
#include <windowsx.h>

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

	m_width = width;
	m_height = height;

	return S_OK;
}

// 赋予空函数，以免在不设置时出错
std::vector<UserDefWndFunc> MainClass::UserFunc = std::vector<UserDefWndFunc>(0);
MouseHandler MainClass::MainMouse;

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
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEWHEEL:
		MainMouse.SetMouseState(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), uMsg);
		for (auto& iter : UserFunc)
			iter(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   MouseHandler   ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void MouseHandler::Init(float x, float y, UINT msg) {
	m_lastX = m_posX = x;
	m_lastY = m_posY = y;
	m_behavior = MOUSE_BEHAVIOR_IDEAL;
	m_lastMsg = msg;

	// 初始化各个状态机
	m_click.SetNextState(&m_ideal);
	m_dclick.SetNextState(&m_ideal);
	m_down.SetNextState(&m_click, &m_drag);
	m_drag.SetNextState(&m_ideal);
	m_ideal.SetNextState(&m_down, &m_dclick);

	m_curState = &m_ideal;
}

void MouseHandler::GetMouseState(float& x, float& y, MOUSE_BEHAVIOR& bhr,
	float* lastX, float* lastY) {
	x = m_posX;
	if (lastX) *lastX = m_lastX;
	y = m_posY;
	if (lastY) *lastY = m_lastY;
	bhr = m_behavior;
}

void MouseHandler::SetMouseState(float x, float y, UINT mouse_msg) {
	m_curState = m_curState->GetNextState(x, y, mouse_msg);

	m_lastX = m_posX;
	m_posX = x;

	m_lastY = m_posY;
	m_posY = y;

	m_lastMsg = mouse_msg;

	m_behavior = m_curState->GetBehavior();
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   MouseStates   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void MouseStateNode::InitNode(float initX, float initY, UINT initMsg) {
	m_initPosX = initX;
	m_initPosY = initY;
	m_initMsg = initMsg;
}

///////////////////
// MouseState Ideal
///////////////////
void MouseState_Ideal::SetNextState(MouseStateNode* down,
	MouseStateNode* dClick) {
	m_down = down;
	m_dClick = dClick;
}

MOUSE_BEHAVIOR MouseState_Ideal::GetBehavior() { return MOUSE_BEHAVIOR_IDEAL; }

MouseStateNode* MouseState_Ideal::GetNextState(float x, float y, UINT msg) {
	switch (msg) {
	case WM_LBUTTONDOWN:
		m_down->InitNode(x, y, msg);
		MLOG(LL, "ideal -> down");
		return m_down;
	case WM_LBUTTONDBLCLK:
		m_dClick->InitNode(x, y, msg);
		MLOG(LL, "ideal -> dclick");
		return m_dClick;
	default:
		return this;
	}
}

///////////////////
// MouseState Down
///////////////////
void MouseState_Down::SetNextState(MouseStateNode* click,
	MouseStateNode* drag) {
	m_click = click;
	m_drag = drag;
}

MouseStateNode* MouseState_Down::GetNextState(float x, float y, UINT msg) {
	switch (msg) {
	case WM_LBUTTONUP:
		m_click->InitNode(x, y, msg);
		MLOG(LL, "down -> click");
		return m_click;
	case WM_MOUSEMOVE:
		m_drag->InitNode(x, y, msg);
		MLOG(LL, "down -> drag");
		return m_drag;
	default:
		return this;
	}
}

MOUSE_BEHAVIOR MouseState_Down::GetBehavior() { return MOUSE_BEHAVIOR_IDEAL; }

///////////////////
// MouseState Drag
///////////////////
void MouseState_Drag::SetNextState(MouseStateNode* ideal) {
	m_ideal = ideal;
}

MouseStateNode* MouseState_Drag::GetNextState(float x, float y, UINT msg) {
	switch (msg) {
	case WM_MOUSEMOVE:
		this->InitNode(x, y, msg);
		return this;
	case WM_LBUTTONUP:
		m_ideal->InitNode(x, y, msg);
		MLOG(LL, "drag -> ideal");
		return m_ideal;
	default:
		return this;
	}
}

MOUSE_BEHAVIOR MouseState_Drag::GetBehavior() { return MOUSE_BEHAVIOR_DRAG; }

///////////////////
// MouseState Click
///////////////////
void MouseState_Click::SetNextState(MouseStateNode* ideal) {
	m_ideal = ideal;
}

MouseStateNode* MouseState_Click::GetNextState(float x, float y, UINT msg) {
	MLOG(LL, "click -> ideal");
	return m_ideal;
}

MOUSE_BEHAVIOR MouseState_Click::GetBehavior() { return MOUSE_BEHAVIOR_CLICK; }

///////////////////
// MouseState DClick
///////////////////
void MouseState_DClick::SetNextState(MouseStateNode* ideal) {
	m_ideal = ideal;
}

MouseStateNode* MouseState_DClick::GetNextState(float x, float y, UINT msg) {
	MLOG(LL, "dclick -> ideal");
	return m_ideal;
}

MOUSE_BEHAVIOR MouseState_DClick::GetBehavior() { return MOUSE_BEHAVIOR_DCLICK; }