#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <map>
#include "RendererProxy.h"
#include "RenderTarget.h"

typedef std::map<UINT, Microsoft::WRL::ComPtr<ID3D11Buffer>>					BufferList;

class UObject;
class Canvas;
class LightProxy;
class InternalTexture;
class IShader;
class RasterState;

class UnknownObject;

class DXRenderer {
public:
	static DXRenderer& GetInstance();
private:
	static D3D11_INPUT_ELEMENT_DESC															Static_InputElementDesc[];
	static D3D11_INPUT_ELEMENT_DESC															Dynamic_InputElementDesc[];
public:
// RenderProxy virtual function
	bool InitSys(HWND, float, float);
	//bool BindModel(Model*);
	//void SetupModel(Model*);

	void Setup(Canvas*);
	void Setup(ITexture*);
	void Setup(LightProxy*);
	void Setup(IShader*);
	void Setup(UnknownObject*);

	void Bind(RenderTargetsDesc*, bool, bool);
	void Bind(ShaderResourcesDesc*);
	void Bind(LightProxy*);
	void Bind(IShader*);
	void Bind(UnknownObject*);

	void Unbind(RenderTargetsDesc*);
	void Unbind(ShaderResourcesDesc*);
	void Unbind(LightProxy*);
	void Unbind(IShader*);
	void Unbind(UnknownObject*);

	void ClearRenderTarget(IRenderTarget*);
	void ClearDepthStencil(ID3D11DepthStencilView*);

	void SetRenderState(RasterState*, D3D11_VIEWPORT* viewport = NULL);
	void EndRender();

	ID3D11Device** GetDevice();
	ID3D11DeviceContext** GetDeviceContext();
	IRenderTarget* GetMainRT();
	ID3D11DepthStencilView* GetMainDepthStencilBuffer();
	ITexture* GetMainDS();

	void IterateFuncs();
	void AddIterateObject(IterateObject*);

private:
	DXRenderer();
	void clearRenderTargets();

	bool setReferenceDev(HWND hwnd, float width, float height);
	bool setBackBuffer(HWND hwnd, float width, float height);
	bool setDepthStencil(HWND hwnd, float width, float height);
	bool setRenderState(HWND hwnd, float width, float height);

	// temporary function
	void setInputLayout();
private:

private:
	std::vector<IterateObject*>																			m_iterateList;
	Microsoft::WRL::ComPtr<ID3D11Device>													m_dev;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>										m_devContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>													m_swapChain;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>									m_depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>												m_depthStencilTexture;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>											m_comInputLayout;

	D3D11_VIEWPORT																						m_viewport;
	CommonRenderTarget																					m_mainRenderTarget;
	std::shared_ptr<InternalTexture>																	m_mainDSResource;
	BufferList																										m_bufferList;
};