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
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, 
		{D2D1_DEBUG_LEVEL_INFORMATION},
		m_factory.ReleaseAndGetAddressOf());
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

BaseUI::BaseUI(RectF _area, BaseUI* _parent)
 :parent(_parent), left(nullptr), right(nullptr),
 area(_area), reDraw(true){
	// TODO: 需要修改父节点中子节点的左右指针！
}

void BasicWindow::Draw() {
	// TODO: 利用Direct2D绘制窗口
}

// 绘制内容全部放在backbuffer中
// 固定UI——只有需要绘制的UI才更行缓冲区
// 双缓冲，如何保障当前绘制的内容和front buffer的一致！
// 消息队列——鼠标，键盘，窗口变更等输入，决定哪些空间需要重新绘制
// 绘制队列——将需要绘制的内容放入队列中，假如绘制任务为空，
//		则当前不需要反转back buffer!

// 所以需要更改当前的结果
// 目前的结构是由窗口循环进行back buffer的翻转
// 窗口循环需要用于更新应用内部数据——比如算法产生渲染结果的更新
//		需要通知UI进行重绘，应用内部数据更新也需要通知UI进行重绘
// 故还需要一个应用数据更新与UI数据更新之间的双向绑定
// 系统消息回调需要接收鼠标键盘输入更新UI状态
// 即当前存在两个通道会更新UI渲染队列，则返还back buffer应该在哪里完成
// 应该在当前再无内容需要添加入绘制队列后，back buffer反转
// 消息循环与系统回调哪一个会最后一个完成？
