#pragma once
#include "../UVConfig.h"
#include "../Utility/QuadTree/QuadTree.h"
#include <list>

namespace UVUI {
	enum Element_Type {
		Element_Type_Slider,
		Element_Type_Frame,
		Element_Type_Label,
		Element_Type_Layer
	};

	enum Cursor_Behavior {
		Cursor_Behavior_Ideal = 1,
		Cursor_Behavior_LBDown = 2,
		Cursor_Behavior_LBUp = 4,
		Cursor_Behavior_Click = 8,
		Cursor_Behavior_RBDown = 16,
		Cursor_Behavior_RBUp = 32,
		Cursor_Behavior_Dragging = 64,
		Cursor_Behavior_Moving = 128,
		Cursor_Behavior_Scrolling = 256
	};
	
	struct UIColor {
		float r, g, b, a;
		UIColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f)
			: r(r), g(g), b(b), a(a) {}
		UIColor(float v) : r(v), g(v), b(v), a(1.0f) {}
		UIColor& operator+(const UIColor& rhs) {
			return { r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a };
		}
	};

	const UIColor UI_Color_Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	const UIColor UI_Color_Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	const UIColor UI_Color_Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	const UIColor UI_Color_Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	const UIColor UI_Color_White = { 1.0f, 1.0f, 1.0f, 1.0f };
	const UIColor UI_Color_Transparent = { 0.0f, 0.0f, 0.0f, 0.0f };

	struct UIRect {
		float left, top;
		float width, height;
		UIRect(float left = 0, float top = 0,
			float width = 0, float height = 0) :
			left(left), top(top), width(width), height(height) {}
		operator LQT::QTRect&() const { return {left, left + width, top, top + height}; }
	};

	struct UIPnt {
		float x, y;
		UIPnt(float x, float y) : x(x), y(y) {}
		operator LQT::QTPoint&() const { return { x, y }; }
	};

	struct UIEvent {
		UIPnt cursorCurPos;
		UIPnt cursorOffset;
		UINT cursorBehavior;
		float deltaTime;
	};

	const float DefaultWidth = 320.0f;
	const float DefaultHeight = 60.0f;
	const float DefaultFontSize = 42.0f;

	class UIElement;
	typedef std::list<UIElement*> UIElementList;
	typedef std::map<const UINT, UIElement*> UIElementMap;
	typedef std::pair<const UINT, UIElement*> UIElementPair;
}
