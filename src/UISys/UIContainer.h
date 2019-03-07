#pragma once
#include "UI_UVConfig.h"


namespace UVUI {

	class UIElement;
	// It's a container that contain elements; responsible for:
	// Adding element; dispatching event; returning element to be draw
	class UIContainer {
	public:
		UIContainer(const UIRect& rect);
		virtual void AddElement(UIElement*);
		virtual UIElement* DispatchEvent(UIEvent&);
		// record elements whose rect(size or pos) has been changed
		// DONT INVOKE IT MANUALLY!
		void ElementRectChange(const UIElement*);
		void RegisterDraw(UIElement*);
		void RegisterEventListener(UIElement*);

		// return elements which has been updated and need to be draw
		virtual UIElementList Draw();
		// return all elements to UISYS to draw itself
		virtual UIElementList DrawAll();
	protected:
		LQT::QuadTree			m_qtree;
		UIElementMap			m_elements;
		UIElementMap			m_forceEventListeners;
		UIElementMap			m_drawList;
		UIRect						m_rect;
		std::vector<LQT::QNodeEle>			m_rectUpdateList;
	};

	class UIPage : public UIContainer {
	public:
		UIPage(const UIRect& rect) : UIContainer(rect) {}
	};

	class DefaultPage : public UIPage {
	public:
		DefaultPage(UIRect rect) : UIPage(rect) {}
		void AddElement(UIElement*);
	private:
		// calculating current page's ui elements position
		void layout();
	private:
		UIRect m_curTotalSize;
	};

	class UserParamPage : public UIPage {
	public:
		UserParamPage(UIRect rect) : UIPage(rect) {}
		void AddController(UIElement*);
	};
};