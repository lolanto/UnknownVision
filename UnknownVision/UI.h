#pragma once
#include <d2d1.h>
#include <dxgi.h>
#include <wrl.h>
#include <list>
#include <dwrite.h>

#include "QuadTree\QTree.h"

// 这只是UI的渲染器
// 还需要设计UI的结构(事件分发，响应和处理等！)

const WCHAR DEFAULT_FONT_FAMILY[] = L"Georgia";

class UIRenderer;
class UISystem;
enum UI_EVENT {
	UI_EVENT_MOUSE_HOVER,
	UI_EVENT_MOUSE_CLICK,
	UI_EVENT_MOUSE_DCLICK,
	UI_EVENT_MOUSE_DRAG
};

class BaseUI : public QLeave {
public:
	BaseUI(RectF _area = {0, 0, 0, 0});
public:
	BaseUI*						parent;
	std::list<BaseUI*>			childs;
	bool						isNeedDraw;
	UINT						zOrder;
public:
	virtual void EventHandle(UI_EVENT, UISystem&);
	virtual void Draw(UIRenderer*) {};
};

class UIRenderer {
public:
	static UIRenderer& GetInstance();
public:
	void Init(IDXGISurface*&);
	void RenderText(std::wstring&, IDWriteTextFormat*, RectF, ID2D1SolidColorBrush*);
	void RenderRectangle(RectF&, ID2D1Brush*, float = 1.0f, ID2D1StrokeStyle* = nullptr);
	void RenderRoundedRectangle(RectF&, ID2D1Brush*, float = 1.0f, ID2D1StrokeStyle* = nullptr);
	void FilledRectangle(RectF&, ID2D1Brush*);
	void FilledRoundedRectangle(RectF&, ID2D1Brush*);

	void StartRender();
	void EndRender();
private:
	UIRenderer();
private:
	bool											m_hasInit;
	Microsoft::WRL::ComPtr<ID2D1Factory>			m_factory;
	Microsoft::WRL::ComPtr<ID2D1RenderTarget>		m_renderTarget;
	// about DirectX Write
	Microsoft::WRL::ComPtr<IDWriteFactory>			m_writeFactory;
};

class UISystem {
public:
	static BaseUI RootUINode;
	static UISystem& GetInstance();
public:
	// UI节点的增加
	void Attach(BaseUI* child);
	void Attach(BaseUI* child, BaseUI* parent);
	// Windows事件处理函数
	void TaskProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	// 强行重绘某个UI
	void RedrawUI(BaseUI*);
	// 让某个UI及其父亲节点至于顶层
	void SetToTop(BaseUI*);
	// 绘制UI的接口
	void Draw();
	// 初始化系统
	void Init();
private:
	// 遍历UI树以及调用渲染方法
	void traverseUITree(BaseUI* root, bool drawFlag, UIRenderer*);
	// 将输入节点移到父节点的最右边
	void moveNodeToRight(BaseUI* input);
private:
	QTree											m_qTree;
	UINT											m_curZOrder;
};

// 不同类型的UI应该对某几个UI事件做出不同类型的响应

class BasicWindow 
	:public BaseUI
{
public:
	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>	DefaultBrush;
	static bool InitBasicWindow(IDWriteFactory*, ID2D1RenderTarget*);
public:
	BasicWindow(RectF _area);
public:
	void Draw(UIRenderer*);
};

class TextCtrl : public BaseUI
{
public:
	// 字体默认格式
	static Microsoft::WRL::ComPtr<IDWriteTextFormat>	DefaultFormat;
	// 字体颜色
	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>	DefaultBrush;
	static bool InitTextCtrl(IDWriteFactory*, ID2D1RenderTarget*);
public:
	TextCtrl(std::wstring str, RectF _area);
	std::wstring content;
public:
	void Draw(UIRenderer*);
};
