#pragma once
#include <d2d1.h>
#include <dxgi.h>
#include <wrl.h>
#include "QuadTree\QTree.h"

const WCHAR DEFAULT_FONT_FAMILY[] = L"Georgia";

class UIRenderer {
public:
	static UIRenderer& GetInstance();
public:
	void Init(IDXGISurface*&);
	void RenderText(std::wstring&, float size, RectF, D2D_COLOR_F);
	void RenderRectangle(RectF&, ID2D1Brush*, float = 1.0f, ID2D1StrokeStyle* = nullptr);
	void RenderRoundedRectangle(RectF&, ID2D1Brush*, float = 1.0f, ID2D1StrokeStyle* = nullptr);
	void FilledRectangle(RectF&, D2D_COLOR_F);
	void FilledRoundedRectangle(RectF&, D2D_COLOR_F, float angle = 0.1f);

	void StartRender();
	void EndRender();

	void Test();

	PntF DPI;

private:
	UIRenderer();
private:
	bool																	m_hasInit;
	Microsoft::WRL::ComPtr<ID2D1Factory>			m_factory;
	Microsoft::WRL::ComPtr<ID2D1RenderTarget>	m_renderTarget;
	// about DirectX Write
	Microsoft::WRL::ComPtr<IDWriteFactory>			m_writeFactory;

	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> m_bitMapTarget;

	// Def Text Data
	struct {
		Microsoft::WRL::ComPtr<IDWriteTextFormat>	  textFormat;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush;
		D2D_COLOR_F													  color;
		float																	  size;
	} m_textData;

	// Def Rectangle Data
	struct {
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> rectBrush;
		D2D_COLOR_F													  color;
	} m_rectData;
};