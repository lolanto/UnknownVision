#pragma once
#include <d2d1.h>
#include <dxgi.h>
#include <wrl.h>

// 这只是UI的渲染器
// 还需要设计UI的结构(事件分发，响应和处理等！)

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
