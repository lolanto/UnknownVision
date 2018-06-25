#pragma once

#include <Windows.h>
#include <string>
#include <functional>
#include <vector>

typedef std::function<void(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)> UserDefWndFunc;

enum MOUSE_BEHAVIOR {
	MOUSE_BEHAVIOR_IDEAL = 0,
	MOUSE_BEHAVIOR_CLICK,
	MOUSE_BEHAVIOR_DCLICK,
	MOUSE_BEHAVIOR_DRAG
};

class MouseStateNode {
public:
	void InitNode(float initX, float initY, UINT initMsg);
	virtual MouseStateNode* GetNextState(float x, float y, UINT msg) = 0;
	virtual MOUSE_BEHAVIOR GetBehavior() = 0;
protected:
	float			m_initPosX;
	float			m_initPosY;
	UINT			m_initMsg;
};

class MouseState_Ideal : public MouseStateNode {
public:
	void SetNextState(MouseStateNode* down,
		MouseStateNode* dClick);
	MouseStateNode* GetNextState(float x, float y, UINT msg);
	MOUSE_BEHAVIOR GetBehavior();
private:
	MouseStateNode *m_down, *m_dClick;
};

class MouseState_Down : public MouseStateNode {
public:
	void SetNextState(MouseStateNode* click,
		MouseStateNode* drag);
	MouseStateNode* GetNextState(float x, float y, UINT msg);
	MOUSE_BEHAVIOR GetBehavior();
private:
	MouseStateNode *m_click, *m_drag;
};

class MouseState_Drag : public MouseStateNode {
public:
	void SetNextState(MouseStateNode* ideal);
	MouseStateNode* GetNextState(float x, float y, UINT msg);
	MOUSE_BEHAVIOR GetBehavior();
private:
	MouseStateNode * m_ideal;
};

class MouseState_Click : public MouseStateNode {
public:
	void SetNextState(MouseStateNode* ideal);
	MouseStateNode* GetNextState(float x, float y, UINT msg);
	MOUSE_BEHAVIOR GetBehavior();
private:
	MouseStateNode * m_ideal;
};

class MouseState_DClick : public MouseStateNode {
public:
	void SetNextState(MouseStateNode* ideal);
	MouseStateNode* GetNextState(float x, float y, UINT msg);
	MOUSE_BEHAVIOR GetBehavior();
private:
	MouseStateNode * m_ideal;
};

class MouseHandler {
public:
	void Init(float x = 0, float y = 0, UINT msg = 0);
	void GetMouseState(float& x, float& y, MOUSE_BEHAVIOR& bhr, 
		float* lastX = nullptr, float* lastY = nullptr);
	void SetMouseState(float x, float y, UINT mouse_msg);
private:
	float			m_posX;
	float			m_posY;
	float			m_lastX;
	float			m_lastY;
	MOUSE_BEHAVIOR  m_behavior;
	UINT			m_lastMsg;

	MouseStateNode*		m_curState;
	// 鼠标状态机状态对象
	MouseState_Click	m_click;
	MouseState_DClick	m_dclick;
	MouseState_Down		m_down;
	MouseState_Drag		m_drag;
	MouseState_Ideal	m_ideal;
};

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
		MainMouse.Init();
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
	static std::vector<UserDefWndFunc> UserFunc;
	static MouseHandler MainMouse;
private:
	HWND m_hWnd;
};

static HINSTANCE m_hInstance;
static std::string m_windowClassName;
