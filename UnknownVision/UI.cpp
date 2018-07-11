#include "UI.h"
#include "InfoLog.h"
#include "MainClass.h"
#include <iostream>
#include <assert.h>
#include <time.h>
#include <windowsx.h>

extern const float WIDTH;
extern const float HEIGHT;

template<typename T>
void EasyExchange(T& a, T& b) {
	T temp = a;
	a = b;
	b = temp;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   BaseUI   ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

BaseUI::BaseUI(RectF _area)
	:QLeave(_area),
	parent(nullptr), isNeedDraw(true) {
	// 每一个初始化的UI都需要被绘制所以isNeedDraw是true
}

void BaseUI::EventHandle(UI_EVENT e, UISystem& sys) {
	if (e == UI_EVENT_MOUSE_CLICK) sys.SetToTop(this);
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   UIRender   ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

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
		{D2D1_DEBUG_LEVEL_INFORMATION},
		m_factory.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "Create D2D Factory failed!");
		return;
	}

	// 准备RenderTarget
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

	// 准备默认字体格式
	if (!LabelCtrl::InitTextCtrl(m_writeFactory.Get(), m_renderTarget.Get())) return;
	if (!BasicWindow::InitBasicWindow(m_writeFactory.Get(), m_renderTarget.Get())) return;

	m_hasInit = true;
}

inline void UIRenderer::StartRender() {
	m_renderTarget->BeginDraw();
}

inline void UIRenderer::EndRender() {
	m_renderTarget->EndDraw();
}

void UIRenderer::test()
{
	m_renderTarget->CreateCompatibleRenderTarget(m_bitMapTarget.ReleaseAndGetAddressOf());
}

void UIRenderer::test2()
{
	m_bitMapTarget->BeginDraw();
	m_bitMapTarget->Clear({1.0, 1.0, 1.0, 0.0});
	m_bitMapTarget->DrawRectangle({ 0, 0, 30, 30 }, BasicWindow::DefaultBrush.Get());
	m_bitMapTarget->EndDraw();

	ID2D1Bitmap* tempBit;
	m_bitMapTarget->GetBitmap(&tempBit);

	m_renderTarget->BeginDraw();
	m_renderTarget->Clear();
	m_renderTarget->DrawBitmap(tempBit);
	m_renderTarget->EndDraw();

	tempBit->Release();
}

inline void UIRenderer::RenderText(std::wstring& str, IDWriteTextFormat* format, RectF area, ID2D1SolidColorBrush* brush) {
	m_renderTarget->DrawTextA(str.c_str(), str.length(), format, area, brush);
}

inline void UIRenderer::RenderRectangle(RectF& area, ID2D1Brush* brush, float stroke, ID2D1StrokeStyle* strokeStyle) {
	m_renderTarget->DrawRectangle(area, brush, stroke, strokeStyle);
}

inline void UIRenderer::RenderRoundedRectangle(RectF& area, ID2D1Brush* brush, float stroke, ID2D1StrokeStyle* strokeStyle) {
	m_renderTarget->DrawRoundedRectangle({ area , 3.0f, 3.0f}, brush, stroke, strokeStyle);
}
inline void UIRenderer::FilledRectangle(RectF& area, ID2D1Brush* brush) {
	m_renderTarget->FillRectangle(area, brush);
}

inline void UIRenderer::FilledRoundedRectangle(RectF& area, ID2D1Brush* brush) {
	m_renderTarget->FillRoundedRectangle({ area, 3.0f, 3.0f }, brush);
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   UISystem   ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
BaseUI UISystem::RootUINode;

///////////////////
// public function
///////////////////

UISystem& UISystem::GetInstance() {
	static UISystem _instance;
	return _instance;
}

void UISystem::Init() {
	// 初始化QT树
	m_qTree = QTree({ 0, 0, WIDTH, HEIGHT }, 4);
}

void UISystem::Attach(BaseUI* child) {
	RootUINode.childs.push_back(child);
	child->parent = &RootUINode;
	m_qTree.Insert(child);
}

void UISystem::Attach(BaseUI* child, BaseUI* parent) {
	parent->childs.push_back(child);
	child->parent = parent;
	m_qTree.Insert(child);
}

void UISystem::Draw() {
	// 开始渲染
	UIRenderer::GetInstance().StartRender();
	m_curZOrder = 0;
	traverseUITree(&RootUINode, false, &UIRenderer::GetInstance());
	UIRenderer::GetInstance().EndRender();
}

void UISystem::SetToTop(BaseUI* ui) { moveNodeToRight(ui); }

void UISystem::TaskProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// 确定该行为是什么类型的操作
	UI_EVENT eveType;
	std::vector<QLeave*> container;
	UINT maxOrder = 0;
	BaseUI* targetUI = nullptr;
	PntF curPos, lastPos;
	MOUSE_BEHAVIOR behavior;
	MainClass::MainMouse.GetMouseState(curPos.x, curPos.y, behavior,
		&lastPos.x, &lastPos.y);
	switch (behavior) {
	// 根据不同类型的鼠标行为选择UI的处理方式
	case MOUSE_BEHAVIOR_CLICK:
		if (m_qTree.CollisionDetect(curPos, container)) {
			for (auto& iter : container) {
				BaseUI* uiPt = static_cast<BaseUI*>(iter);
				if (maxOrder < uiPt->zOrder) {
					maxOrder = uiPt->zOrder;
					targetUI = uiPt;
				}
			}
			if (targetUI) targetUI->EventHandle(UI_EVENT_MOUSE_CLICK, *this);
		}
		break;
	default:
		break;
	}
	// 确定该行为触发的UI元素
	// 确定该UI元素是否需要重新绘制
}

void UISystem::RedrawUI(BaseUI* ui) {
	// 设置UI在渲染树中的状态为需要渲染
	ui->isNeedDraw = true;
	std::vector<QLeave*> container = std::vector<QLeave*>(0);
	if (m_qTree.CollisionDetect(ui->area, container)) {
		for (auto& iter : container) {
			BaseUI* uiPt = static_cast<BaseUI*>(iter);
			uiPt->isNeedDraw = true;
		}
	}
}

///////////////////
// private function
///////////////////

void UISystem::traverseUITree(BaseUI* root, bool drawFlag, UIRenderer* renderer) {
	root->zOrder = m_curZOrder++;
	if (drawFlag) {
		root->Draw(renderer);
		root->isNeedDraw = false;
		for (auto& iter : root->childs)
			traverseUITree(iter, true, renderer);
	}
	else {
		if (root->isNeedDraw) {
			root->Draw(renderer);
			root->isNeedDraw = false;
			for (auto& iter : root->childs)
				traverseUITree(iter, true, renderer);
		}
		else {
			for (auto& iter : root->childs)
				traverseUITree(iter, false, renderer);
		}
	}
}

void UISystem::moveNodeToRight(BaseUI* input) {
	BaseUI* parent = input->parent;
	parent->childs.remove(input);
	parent->childs.push_back(input);
	if (input->parent != &RootUINode)
		moveNodeToRight(parent);
	else
		input->isNeedDraw = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Basic Window   ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> BasicWindow::DefaultBrush;

inline bool BasicWindow::InitBasicWindow(IDWriteFactory* factory, ID2D1RenderTarget* rt) {
	rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray), DefaultBrush.ReleaseAndGetAddressOf());
	return true;
}

BasicWindow::BasicWindow(RectF _area)
	:BaseUI(_area) {}

void BasicWindow::Draw(UIRenderer* renderer) {
	renderer->FilledRoundedRectangle(BaseUI::area, DefaultBrush.Get());
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   TextCtrl   //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

Microsoft::WRL::ComPtr<IDWriteTextFormat> LabelCtrl::DefaultFormat;
Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> LabelCtrl::DefaultBrush;

inline bool LabelCtrl::InitTextCtrl(IDWriteFactory* factory, ID2D1RenderTarget* rt) {
	HRESULT hr = factory->CreateTextFormat(
		DEFAULT_FONT_FAMILY,
		nullptr,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		72.0f,
		L"en-us",
		DefaultFormat.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, __FUNCTION__, LL, "Create default text format failed!");
		return false;
	}
	DefaultFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	DefaultFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),
		DefaultBrush.ReleaseAndGetAddressOf());

	return true;
}

LabelCtrl::LabelCtrl(std::wstring str, RectF area) 
	:BaseUI(area), content(str) {}

void LabelCtrl::Draw(UIRenderer* renderer) {
	renderer->RenderText(content, DefaultFormat.Get(), BaseUI::area, DefaultBrush.Get());
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
