#pragma once
#include <d2d1.h>
#include <dxgi.h>
#include <wrl.h>

// ��ֻ��UI����Ⱦ��
// ����Ҫ���UI�Ľṹ(�¼��ַ�����Ӧ�ʹ���ȣ�)

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

class IUIElement {
public:
	virtual void HandleMouseMove(float x, float y, UINT timeStamp, int param) {}
	virtual void HandleMouseDown(float x, float y, UINT timeStamp, int param) {}
	virtual void HandleMouseUp(float x, float y, UINT timeStamp, int param) {}
	virtual void HandleKeyDown() {}
};
