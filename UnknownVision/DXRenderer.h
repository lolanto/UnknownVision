#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <map>
#include <memory>
#include "RasterState.h"

#define MainDev DXRenderer::GetInstance().GetDevice()
#define MainDevCtx DXRenderer::GetInstance().GetContext()

typedef std::map<UINT, Microsoft::WRL::ComPtr<ID3D11Buffer>>					BufferList;

class RasterState;
class RenderTargetWrapper;
class DepthTexture;
class IRenderTarget;
class IUnorderAccess;

class UnknownObject;

class DXRenderer {
public:
	static DXRenderer& GetInstance();
private:
	static D3D11_INPUT_ELEMENT_DESC															Static_InputElementDesc[];
	static D3D11_INPUT_ELEMENT_DESC															Dynamic_InputElementDesc[];
public:
	bool InitSys(HWND, float, float);
	void EndRender();

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetContext();

	RenderTargetWrapper* GetMainRT();
	DepthTexture* GetMainDS();
	// 单独提供渲染对象的清空
	void ClearRenderTarget(IRenderTarget* rt, DirectX::XMFLOAT4 clearColor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) );
	void ClearUAV_UINT(IUnorderAccess* uav, DirectX::XMUINT4 clearValue = DirectX::XMUINT4(0, 0, 0, 0));
	void ClearUAV_FLOAT(IUnorderAccess* uav, DirectX::XMFLOAT4 clearValue = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
private:
	DXRenderer();

	bool setReferenceDev(HWND hwnd, float width, float height);
	bool setBackBuffer(HWND hwnd, float width, float height);
	bool setDepthStencil(HWND hwnd, float width, float height);
	bool setRenderState(HWND hwnd, float width, float height);

	// temporary function
	void setInputLayout();
private:

private:
	//std::vector<IterateObject*>																			m_iterateList;
	Microsoft::WRL::ComPtr<ID3D11Device>													m_dev;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>										m_devContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>												m_swapChain;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>											m_comInputLayout;
	RasterState																									m_defRasterState;
	D3D11_VIEWPORT																						m_defViewport;
	std::shared_ptr<RenderTargetWrapper>														m_mainRenderTarget;
	std::shared_ptr<DepthTexture>																	m_mainDepthTexture;
};