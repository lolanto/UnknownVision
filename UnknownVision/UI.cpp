#include "UI.h"
#include "InfoLog.h"

UIRenderer& UIRenderer::GetInstance() {
	static UIRenderer _instance;
	return _instance;
}

UIRenderer::UIRenderer() : m_hasInit(false) {}

///////////////////
// public function
///////////////////
void UIRenderer::Init(IDXGISurface*& sur) {
	const char* funcTag = "UIRenderer::Init: ";
	float dpiX = 0, dpiY = 0;
	HRESULT hr = S_OK;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_factory.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LL, "Create D2D Factory failed!");
		return;
	}
	m_factory.Get()->GetDesktopDpi(&dpiX, &dpiY);

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
		MLOG(LL, funcTag, LL, "Create D2D Render Target failed!");
		return;
	}

	m_hasInit = true;
}

void UIRenderer::Prepare() {
	if (!m_hasInit) return;
	m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),
		m_brush.ReleaseAndGetAddressOf());
}

void UIRenderer::Draw() {
	if (!m_hasInit) return;
	m_renderTarget->BeginDraw();
	m_renderTarget->DrawRectangle(
		D2D1::RectF(0, 0, 100, 100), m_brush.Get()
	);
	m_renderTarget->EndDraw();
}
