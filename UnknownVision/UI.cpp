#include "UI.h"
#include "InfoLog.h"
#include "MainClass.h"
#include <iostream>
#include <assert.h>
#include <time.h>
#include <windowsx.h>
#include "UIRenderer.h"

extern const float WIDTH;
extern const float HEIGHT;
UINT UIID = 0;
using namespace UVUI;

template<typename T>
void EasyExchange(T& a, T& b) {
	T temp = a;
	a = b;
	b = temp;
}

// return a !=b
bool inline ColorComp(D2D_COLOR_F a, D2D_COLOR_F b) {
	return (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a);
}

/*============
UVUI::EventHandler
============*/
EventHandler::EventHandler()
	:click(nullptr), hover(nullptr), drag(nullptr) {}

// Default event handler, handle nothing
void EventHandler::HandleEvent(EVENT_DATA*) { return; }

void EventHandler::SetEvent(EVENT_TYPE type, EventFunc func) {
	switch (type) {
	case EVENT_MOUSE_CLICK:
		click = func;
		break;
	case EVENT_MOUSE_HOVER:
		hover = func;
		break;
	case EVENT_MOUSE_DRAG:
		drag = func;
		break;
	default:
		MLOG(LL, __FUNCTION__, LL, "there's no function to handle this event!");
		break;
	}
}

/*============
UIObject
============*/
UIObject::UIObject(RectF area, bool visible)
	: QLeave(area), RenderObject(visible), EventHandler() {
	id = UIID++;
	DispatchDrawRequest();
}

void UIObject::DispatchDrawRequest() {
	if (!redraw) {
		UISystem::GetInstance().AddDrawRequest(this);
		redraw = true;
	}
}

/*============
Label
============*/
Label::Label(std::wstring text,
	RectF area, D2D_COLOR_F txtColor, D2D_COLOR_F bkgColor)
	: m_text(text),m_txtColor(txtColor),
	m_bkgColor(bkgColor), UIObject(area) {}

void Label::SetText(std::wstring text) {
	if (m_text == text) return;
	m_text = text;
	DispatchDrawRequest();
}

void Label::SetBkgColor(D2D_COLOR_F color) {
	if (!ColorComp(color, m_bkgColor)) return;
	m_bkgColor = color;
	DispatchDrawRequest();
}

void Label::SetTxtColor(D2D_COLOR_F color) {
	if (!ColorComp(color, m_txtColor)) return;
	m_txtColor = color;
	DispatchDrawRequest();
}

void Label::Draw() {
	if (!redraw) return;
	UIRenderer& renderer = UIRenderer::GetInstance();
	renderer.FilledRoundedRectangle(area, m_bkgColor);
	float width = area.right - area.left;
	float size = width * 48.0f / renderer.DPI.x / m_text.size();
	renderer.RenderText(m_text, size, area, m_txtColor);
	redraw = false;
}

void Label::HandleEvent(EVENT_DATA* ed) {
	switch (ed->type)
	{
	case EVENT_MOUSE_CLICK:
		if (click) click(ed);
		break;
	default:
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   UISystem   ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

///////////////////
// public function
///////////////////

UISystem& UISystem::GetInstance() {
	static UISystem _instance;
	return _instance;
}

void UISystem::Init() {
	// 初始化QT树
	m_qTree = QTree({ 0, 0, WIDTH, HEIGHT }, 4);
}

void UISystem::AddUI(UIObject* ui) {
	m_qTree.Insert(ui);
}

void UISystem::AddDrawRequest(UIObject* ui) {
	m_drawList.push_back(ui);
}

void UISystem::Draw() {
	UIRenderer::GetInstance().StartRender();
	for (auto& iter : m_drawList)
		iter->Draw();
	m_drawList.clear();
	UIRenderer::GetInstance().EndRender();
}

void UISystem::SysEventProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// 确定该行为是什么类型的操作
	EVENT_TYPE eveType;
	std::vector<QLeave*> container;
	UINT maxOrder = 0;
	UIObject* targetUI = nullptr;
	PntF curPos, lastPos;
	MOUSE_BEHAVIOR behavior;
	MainClass::MainMouse.GetMouseState(curPos.x, curPos.y, behavior,
		&lastPos.x, &lastPos.y);
	switch (behavior) {
	// 根据不同类型的鼠标行为选择UI的处理方式
	case MOUSE_BEHAVIOR_CLICK:
		m_qTree.CollisionDetect(curPos, container);
		EVENT_DATA data;
		data.type = EVENT_MOUSE_CLICK;
		data.mouseCurPOS = curPos;
		data.mouseOffsetPos = { curPos.x - lastPos.x, curPos.y - lastPos.y };
		for (auto& iter : container) {
			UIObject* eh = static_cast<UIObject*>(iter);
			eh->HandleEvent(&data);
		}
	default:
		break;
	}
	// 确定该行为触发的UI元素
	// 确定该UI元素是否需要重新绘制
}

// 绘制内容全部放在backbuffer中
// 固定UI——只有需要绘制的UI才更行缓冲区
// 双缓冲，如何保障当前绘制的内容和front buffer的一致！
// 消息队列——鼠标，键盘，窗口变更等输入，决定哪些空间需要重新绘制
// 绘制队列——将需要绘制的内容放入队列中，假如绘制任务为空，
//		则当前不需要反转back buffer!

// 所以需要更改当前的结果
// 目前的结构是由窗口循环进行back buffer的翻转
// 窗口循环需要用于更新应用内部数据——比如算法产生渲染结果的更新
//		需要通知UI进行重绘，应用内部数据更新也需要通知UI进行重绘
// 故还需要一个应用数据更新与UI数据更新之间的双向绑定
// 系统消息回调需要接收鼠标键盘输入更新UI状态
// 即当前存在两个通道会更新UI渲染队列，则返还back buffer应该在哪里完成
// 应该在当前再无内容需要添加入绘制队列后，back buffer反转
// 消息循环与系统回调哪一个会最后一个完成？
