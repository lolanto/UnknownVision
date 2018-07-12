#pragma once
#include <list>
#include <dwrite.h>
#include <functional>
#include "QuadTree\QTree.h"
#include "Canvas.h"
#include "Pass.h"
#include "Shader.h"
#include <d2d1.h>

// 这只是UI的渲染器
// 还需要设计UI的结构(事件分发，响应和处理等！)

/*=====================
 ! Unknown Vision User Interface !
 ====================*/
namespace UVUI {

	enum EVENT_TYPE {
		EVENT_IDEL = 0,
		EVENT_MOUSE_CLICK,
		EVENT_MOUSE_HOVER,
		EVENT_MOUSE_DRAG
	};

	struct EVENT_DATA {
		EVENT_TYPE		type;
		PntF					mouseCurPOS; // 鼠标当前位置
		PntF					mouseOffsetPos; // 鼠标位置偏移值
	};

	typedef std::function<void(EVENT_DATA*)> EventFunc;

	class RenderObject {
	public:
		RenderObject(bool _visible = true) :visible(visible),redraw(false)  {}
		virtual void Draw() = 0;
	public:
		bool				visible;
		bool				redraw;
	};

	class EventHandler {
	public:
		EventHandler();
		virtual void HandleEvent(EVENT_DATA*);
		void SetEvent(EVENT_TYPE, EventFunc);
	protected:
		EventFunc		click;
		EventFunc		hover;
		EventFunc		drag;
	};

	class UIObject :
		public QLeave,
		public RenderObject,
		public EventHandler {
	public:
		UIObject(RectF area, bool visible = true);
	protected:
		UINT id;
		void DispatchDrawRequest();
	};

	class Label : public UIObject {
	public:
		Label(std::wstring text, RectF area, D2D_COLOR_F txtColor = {1.0f, 1.0f, 1.0f, 1.0f}, 
			D2D_COLOR_F bkgColor = {0.0f, 0.0f, 0.0f, 1.0f});
		void SetText(std::wstring text);
		void SetBkgColor(D2D_COLOR_F color);
		void SetTxtColor(D2D_COLOR_F color);
		void HandleEvent(EVENT_DATA*);
		void Draw();
	private:
		std::wstring			m_text;
		D2D_COLOR_F		m_bkgColor;
		D2D_COLOR_F		m_txtColor;
	};


	class FrameBox : public UIObject {
	public:
		FrameBox(RectF area);
		void HandleEvent(EVENT_DATA*);
		void Draw();
	private:
		Canvas				cvs;
		ShadingPass		sp;
		VertexShader	vs;
		PixelShader		ps;
	};

	class Slider : public RenderObject {

	};

	class CheckBox : public RenderObject {

	};

	class UISystem {
	public:
		static UISystem & GetInstance();
		void SysEventProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void Init();
		void AddUI(UIObject*);
		void AddDrawRequest(UIObject*);
		void Draw();
	private:
		UISystem() {}
	private:
		QTree						m_qTree;
		std::list<UIObject*>	m_drawList;
	};
}