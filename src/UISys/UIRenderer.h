#pragma once
#include "UI_UVConfig.h"

namespace UVUI {
	class UIRenderer {
	public:
		// ���
		virtual void DrawTriangle() = 0;
		virtual void DrawRectangle() = 0;
		virtual void DrawCircle() = 0;

		// �����ɫ
		virtual void FilledTriangle() = 0;
		virtual void FilledRectangle() = 0;
		virtual void FilledCircle() = 0;

		// ��������
		virtual void RenderFonts() = 0;
		
		// һ�����
		virtual void Clear(UIColor uc = UI_Color_Transparent) = 0;
		virtual void Clear(UIRect area, UIColor uc = UI_Color_Transparent) = 0;
		// һ������
		void SetRounded(float v) { m_rounded = v; }
	protected:
		float m_rounded; // Բ�ǻ���
	};
}