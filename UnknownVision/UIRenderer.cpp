#include "UIRenderer.h"
#include "InfoLog.h"

// return a !=b
bool inline ColorComp(D2D_COLOR_F a, D2D_COLOR_F b) {
	return (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a);
}

UIRenderer& UIRenderer::GetInstance() {
	static UIRenderer _instance;
	return _instance;
}

UIRenderer::UIRenderer() : m_hasInit(false) {}

///////////////////
// public function
///////////////////
void UIRenderer::Init(IDXGISurface*& sur) {
	float dpiX = 0, dpiY = 0;
	HRESULT hr = S_OK;
	// 准备D2D Factory
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		{ D2D1_DEBUG_LEVEL_INFORMATION },
		m_factory.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "Create D2D Factory failed!");
		return;
	}

	// 准备RenderTarget
	m_factory.Get()->GetDesktopDpi(&dpiX, &dpiY);
	DPI.x = dpiX; DPI.y = dpiY;
	D2D1_RENDER_TARGET_PROPERTIES rtp;
	ZeroMemory(&rtp, sizeof(rtp));
	rtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	rtp.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	rtp.dpiX = dpiX;
	rtp.dpiY = dpiY;
	rtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	rtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;

	hr = m_factory.Get()->CreateDxgiSurfaceRenderTarget(sur, &rtp, m_renderTarget.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "Create D2D Render Target failed!");
		return;
	}

	// 准备DirectWrite
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(m_writeFactory.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "Create DirectX Write Failed!");
		return;
	}
	m_hasInit = true;
}

void UIRenderer::StartRender() {
	m_renderTarget->BeginDraw();
}

void UIRenderer::EndRender() {
	m_renderTarget->EndDraw();
}

void UIRenderer::RenderText(std::wstring& str, float size, RectF area, D2D_COLOR_F color) {
	HRESULT hr = S_OK;
	if (size != m_textData.size) {
		hr = m_writeFactory->CreateTextFormat(DEFAULT_FONT_FAMILY,
			nullptr,
			DWRITE_FONT_WEIGHT_BLACK,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			size,
			L"zh-cn",
			m_textData.textFormat.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			MLOG(LL, __FUNCTION__, LL, "create text format failed!");
			return;
		}
	}
	if (ColorComp(color, m_textData.color)) {
		hr = m_renderTarget->CreateSolidColorBrush(color, m_textData.textBrush.ReleaseAndGetAddressOf());
	}
	m_renderTarget->DrawTextA(str.c_str(), str.length(), m_textData.textFormat.Get(), area, m_textData.textBrush.Get());
}

void UIRenderer::RenderRectangle(RectF& area, ID2D1Brush* brush, float stroke, ID2D1StrokeStyle* strokeStyle) {
	m_renderTarget->DrawRectangle(area, brush, stroke, strokeStyle);
}

void UIRenderer::RenderRoundedRectangle(RectF& area, ID2D1Brush* brush, float stroke, ID2D1StrokeStyle* strokeStyle) {
	m_renderTarget->DrawRoundedRectangle({ area , 3.0f, 3.0f }, brush, stroke, strokeStyle);
}
void UIRenderer::FilledRectangle(RectF& area, D2D_COLOR_F color) {
	HRESULT hr;
	if (ColorComp(color, m_rectData.color)) {
		hr = m_renderTarget->CreateSolidColorBrush(color, m_rectData.rectBrush.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			MLOG(LL, __FUNCTION__, LL, "create solid brush failed!");
			return;
		}
	}
	m_renderTarget->FillRectangle(area, m_rectData.rectBrush.Get());
}

void UIRenderer::FilledRoundedRectangle(RectF& area, D2D_COLOR_F color, float angle) {
	HRESULT hr;
	if (ColorComp(color, m_rectData.color)) {
		hr = m_renderTarget->CreateSolidColorBrush(color, m_rectData.rectBrush.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			MLOG(LL, __FUNCTION__, LL, "create solid brush failed!");
			return;
		}
	}
	m_renderTarget->FillRoundedRectangle({ area, angle, angle }, m_rectData.rectBrush.Get());
}
