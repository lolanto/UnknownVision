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
	// ��ʼ��QT��
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
	// ȷ������Ϊ��ʲô���͵Ĳ���
	EVENT_TYPE eveType;
	std::vector<QLeave*> container;
	UINT maxOrder = 0;
	UIObject* targetUI = nullptr;
	PntF curPos, lastPos;
	MOUSE_BEHAVIOR behavior;
	MainClass::MainMouse.GetMouseState(curPos.x, curPos.y, behavior,
		&lastPos.x, &lastPos.y);
	switch (behavior) {
	// ���ݲ�ͬ���͵������Ϊѡ��UI�Ĵ���ʽ
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
	// ȷ������Ϊ������UIԪ��
	// ȷ����UIԪ���Ƿ���Ҫ���»���
}

// ��������ȫ������backbuffer��
// �̶�UI����ֻ����Ҫ���Ƶ�UI�Ÿ��л�����
// ˫���壬��α��ϵ�ǰ���Ƶ����ݺ�front buffer��һ�£�
// ��Ϣ���С�����꣬���̣����ڱ�������룬������Щ�ռ���Ҫ���»���
// ���ƶ��С�������Ҫ���Ƶ����ݷ�������У������������Ϊ�գ�
//		��ǰ����Ҫ��תback buffer!

// ������Ҫ���ĵ�ǰ�Ľ��
// Ŀǰ�Ľṹ���ɴ���ѭ������back buffer�ķ�ת
// ����ѭ����Ҫ���ڸ���Ӧ���ڲ����ݡ��������㷨������Ⱦ����ĸ���
//		��Ҫ֪ͨUI�����ػ棬Ӧ���ڲ����ݸ���Ҳ��Ҫ֪ͨUI�����ػ�
// �ʻ���Ҫһ��Ӧ�����ݸ�����UI���ݸ���֮���˫���
// ϵͳ��Ϣ�ص���Ҫ�����������������UI״̬
// ����ǰ��������ͨ�������UI��Ⱦ���У��򷵻�back bufferӦ�����������
// Ӧ���ڵ�ǰ����������Ҫ�������ƶ��к�back buffer��ת
// ��Ϣѭ����ϵͳ�ص���һ�������һ����ɣ�
