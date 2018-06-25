#pragma once
#include <d2d1.h>
#include <dxgi.h>
#include <wrl.h>
#include <list>
#include <dwrite.h>

#include "QuadTree\QTree.h"

// ��ֻ��UI����Ⱦ��
// ����Ҫ���UI�Ľṹ(�¼��ַ�����Ӧ�ʹ���ȣ�)

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
	// UI�ڵ������
	void Attach(BaseUI* child);
	void Attach(BaseUI* child, BaseUI* parent);
	// Windows�¼�������
	void TaskProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	// ǿ���ػ�ĳ��UI
	void RedrawUI(BaseUI*);
	// ��ĳ��UI���丸�׽ڵ����ڶ���
	void SetToTop(BaseUI*);
	// ����UI�Ľӿ�
	void Draw();
	// ��ʼ��ϵͳ
	void Init();
private:
	// ����UI���Լ�������Ⱦ����
	void traverseUITree(BaseUI* root, bool drawFlag, UIRenderer*);
	// ������ڵ��Ƶ����ڵ�����ұ�
	void moveNodeToRight(BaseUI* input);
private:
	QTree											m_qTree;
	UINT											m_curZOrder;
};

// ��ͬ���͵�UIӦ�ö�ĳ����UI�¼�������ͬ���͵���Ӧ

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
	// ����Ĭ�ϸ�ʽ
	static Microsoft::WRL::ComPtr<IDWriteTextFormat>	DefaultFormat;
	// ������ɫ
	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>	DefaultBrush;
	static bool InitTextCtrl(IDWriteFactory*, ID2D1RenderTarget*);
public:
	TextCtrl(std::wstring str, RectF _area);
	std::wstring content;
public:
	void Draw(UIRenderer*);
};
