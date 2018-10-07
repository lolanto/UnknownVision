#pragma once
#include "UI_UVConfig.h"

namespace UVUI {

	class UISys;
	class UIContainer;

	class UIElement {
	public:
		UIElement(UINT id, Element_Type type, const UIRect& rect)
			: CID(id), Type(type), m_rect(rect),
			m_container(nullptr),m_isDirty(false) {}
		virtual void HandleEvent(const UIEvent& e) {};
		virtual std::list<UIElement*> Draw() { std::list<UIElement*> re; re.push_back(this); return re; }
	public:
		// setting element's parent, DONT INVOKE IT MANUALLY
		void SetContainer(UIContainer* container) { m_container = container; }
		// invoking by uipage, DONT INVOKE IT MANUALLY
		void ClearDirtyMark() { m_isDirty = false; }
	public:
		void SetPos(const UIPnt& pos); // change elements's position
		void SetSize(const float& width, const float& height); // change elements's w&h
		const UIRect& GetRect() const { return m_rect; }
	public:
		const UINT CID;
		const Element_Type Type;
	protected:
		UIRect			m_rect;
		UIContainer* m_container;
		bool				m_isDirty;
	};

	class Slider : public UIElement {
	public:
		Slider(std::string name, const UIRect& rect, float min, float max, float cur,
			UINT id)
			: UIElement(id, Element_Type_Slider, rect),
		m_name(name), m_minValue(min), m_maxValue(max), m_curValue(cur){}
		// 重载赋值运算符，接受float值直接修改当前值
		Slider& operator=(const float& f) { SetValue(f); }
		operator float() { return GetValue(); }
		void SetValue(const float& f);
		float GetValue() { return m_curValue; }
		void HandleEvent(const UIEvent& e);
	private:
		std::string	m_name;
		float			m_minValue;
		float			m_maxValue;
		float			m_curValue;
	};

	class Label : public UIElement {
	public:
		Label(std::string content, const UIRect& rect,
			UINT id)
			: UIElement(id, Element_Type_Label, rect),
		m_content(content){}
		// 重载赋值运算符，设置标签内容
		Label& operator=(const std::string& s);
		void SetContent(const std::string& s);
		const char* GetContent() { return m_content.c_str(); }
	private:
		std::string m_content;
	};

	class Layer : public UIElement {

	};

	class HorizonContainerScroller : public UIElement {

	};

	class VerticalContainerScroller : public UIElement {

	};

	class HorizonContainer : public UIElement {

	};

	class VerticalContainer : public UIElement {

	};

};