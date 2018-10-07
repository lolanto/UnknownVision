#include "UIContainer.h"
#include "UIElement.h"
using namespace UVUI;
/*==========
! Container !
==========*/
UIContainer::UIContainer(const UIRect& rect) : m_rect(rect), m_qtree(rect) {}

UIElement* UIContainer::DispatchEvent(UIEvent& e) {
	//before dispath event, update quad tree if any rect of the elemnt  in it has been changed
	for (auto& iter : m_rectUpdateList) {
		m_qtree.Erase(iter);
		UIElement*&& ele = reinterpret_cast<UIElement*>(iter.vPtr);
		m_qtree.Insert({ ele->GetRect(), iter.vPtr });
		ele->ClearDirtyMark();
	}
	m_rectUpdateList.clear();
	// query the quad tree with rect or point in event data
	UIElement* retValue = nullptr;
	std::list<LQT::QNodeEle> reList;
	if (m_qtree.Query(e.cursorCurPos, reList)) {
		retValue = reinterpret_cast<UIElement*>(reList.back().vPtr);
		retValue->HandleEvent(e);
	}
	// dispatch event to elements which need to receive other event for determining its state
	for (auto& iter : m_forceEventListeners) {
		iter.second->HandleEvent(e);
	}
	m_forceEventListeners.clear();
	return retValue;
}

void UIContainer::ElementRectChange(const UIElement* ele) {
	// since element will check its dirty mark and
	// make sure each of elements just invoke this function once
	// so there's no need to check the redundancy
	m_rectUpdateList.push_back({ ele->GetRect(), (void*)ele });
}

void UIContainer::RegisterDraw(UIElement* ele) {
	// since each element has unique CID, it's safe to insert multiple times
	m_drawList.insert(std::make_pair(ele->CID, ele));
}

void UIContainer::RegisterEventListener(UIElement* ele) {
	// since each element has unique CID, it's safe to insert multiple times
	m_forceEventListeners.insert(std::make_pair(ele->CID, ele));
}

UIElementList UIContainer::Draw() {
	UIElementList retList;
	for (auto& iter : m_drawList)
		retList.splice(retList.end(), iter.second->Draw());
	m_drawList.clear();
	return retList;
}

UIElementList UIContainer::DrawAll() {
	// since all of elements need to be draw
	// we don't need the drawList which contains must be redraw
	m_drawList.clear();
	UIElementList retList;
	for (auto& iter : m_elements)
		retList.splice(retList.end(), iter.second->Draw());
	return retList;
}

void UIContainer::AddElement(UIElement* ele) {
	m_qtree.Insert({ele->GetRect(), (void*)ele });
	m_elements.insert(std::make_pair(ele->CID, ele));
	ele->SetContainer(this);
	m_drawList.insert(std::make_pair(ele->CID, ele));
}

/*==========
! DefaultPage !
==========*/
void DefaultPage::AddElement(UIElement* ele) {
	// elements accumulate in the middle of the screen
	const UIRect& uir = ele->GetRect();
	// element's size and position need to be set if its width or height is 0
	if (!uir.width || !uir.height) {
		if (ele->Type != Element_Type_Label) ele->SetSize(DefaultWidth, DefaultHeight);
		else {
			// if ui type is label, its width and height is determined by its content
			Label* l = static_cast<Label*>(ele);
			float totalSize = strnlen_s(l->GetContent(), SIZE_MAX) * DefaultFontSize;
			float width = totalSize / 1.618f;
			width = width > m_rect.width * 0.7f ? m_rect.width * 0.7f : width;
			float height = totalSize / width;
			width *= 1.1f; height *= 1.1f;
			ele->SetSize(width, height);
		}
	}
	layout();
	UIPage::AddElement(ele);
}

void DefaultPage::layout() {
	float curTop = 0.0f;
	for (auto& iter : m_elements) {
		UIElement*& ele = iter.second;
		const UIRect& rect = ele->GetRect();
		ele->SetPos({ (m_rect.width - rect.width) / 2.0f,
			curTop });
		curTop += rect.height;
	}
}