#pragma once
#include "UI_UVConfig.h"
#include "UIContainer.h"
#include "UIElement.h"
#include <Windows.h>

namespace UVUI {

	class UIFactory {
	public:
		UIPage* CreatePage();
		Slider CreateSlider();
		Label CreateLabel();
	};

	// It's responsible for invoking functions of UIRenderer
	// to draw each type of ui elements
	class UISys {
	public:
		void SwitchPage(UIPage*);
		// UI�¼�������
		void EventProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		// UI���º���
		void Update();
	private:
		UIPage*					m_curPage;
	};
};