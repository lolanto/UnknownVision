#include "UIElement.h"
#include "UIContainer.h"
using namespace UVUI;

void UIElement::SetPos(const UIPnt& pos) {
	if (!m_isDirty && m_container) {
		m_container->ElementRectChange(this);
		m_container->RegisterDraw(this);
		m_isDirty = true;
	} 
	m_rect.left = pos.x;
	m_rect.top = pos.y;
}

void UIElement::SetSize(const float& width, const float& height) {
	if (!m_isDirty && m_container) {
		m_container->ElementRectChange(this);
		m_container->RegisterDraw(this);
		m_isDirty = true;
	}
	m_rect.width = width;
	m_rect.height = height;
}

/*===========
!Label!
===========*/
Label& Label::operator=(const std::string& s) { SetContent(s); }

void Label::SetContent(const std::string& s) {
	m_content = s;
	if (m_container) m_container->RegisterDraw(this);
}

/*===========
!Slider!
===========*/

void Slider::SetValue(const float& f) {
	float nv = f;
	if (nv > m_maxValue) nv = m_maxValue;
	else if (nv < m_minValue) nv = m_minValue;
	if (nv != m_curValue) {
		m_curValue = nv;
		if (m_container) m_container->RegisterDraw(this);
	}
}

void Slider::HandleEvent(const UIEvent& e) {
	// 0: ideal state, waiting for cursor
	static int state = 0;
	if (e.cursorBehavior & Cursor_Behavior_LBDown && state == 0) {

	}
	else if (e.cursorBehavior & Cursor_Behavior_LBUp && state == 1) {

	}
	else if (e.cursorBehavior & Cursor_Behavior_Moving && state == 1) {

	}
	else { state = 0; }
}