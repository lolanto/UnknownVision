#pragma once
#include "UI_UVConfig.h"

namespace UVUI {
	class UIRenderer {
	public:
		// 描边
		virtual void DrawTriangle() = 0;
		virtual void DrawRectangle() = 0;
		virtual void DrawCircle() = 0;

		// 填充颜色
		virtual void FilledTriangle() = 0;
		virtual void FilledRectangle() = 0;
		virtual void FilledCircle() = 0;

		// 特殊内容
		virtual void RenderFonts() = 0;
		
		// 一般操作
		virtual void Clear(UIColor uc = UI_Color_Transparent) = 0;
		virtual void Clear(UIRect area, UIColor uc = UI_Color_Transparent) = 0;
		// 一般设置
		void SetRounded(float v) { m_rounded = v; }
	protected:
		float m_rounded; // 圆角弧度
	};
}