#pragma once
#include <d2d1.h>
#include <dxgi.h>
#include <wrl.h>
#include <vector>

// ��ֻ��UI����Ⱦ��
// ����Ҫ���UI�Ľṹ(�¼��ַ�����Ӧ�ʹ���ȣ�)

typedef D2D_POINT_2F PntF;
typedef D2D_RECT_F RectF;

class UIRenderer {
public:
	static UIRenderer& GetInstance();
public:
	void Init(IDXGISurface*&);
private:
	UIRenderer();
private:
	bool																			m_hasInit;
	Microsoft::WRL::ComPtr<ID2D1Factory>					m_factory;
	Microsoft::WRL::ComPtr<ID2D1RenderTarget>		m_renderTarget;

// Temp Methods
public:
	void Draw();
	void Prepare();
// Temp resource
private:
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>	m_brush;
};

class BaseUI {
public:
	BaseUI(RectF _area, BaseUI* _parent);
public:
	BaseUI * parent, *left, *right;
	std::vector<BaseUI*> childs;
public:
	RectF							area;
	bool								reDraw;
	virtual void Draw() = 0;
};

class BasicWindow :
	public BaseUI
{
	void Draw();
};
