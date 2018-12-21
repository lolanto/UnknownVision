#include "../RenderSys/RenderSys.h"
#include "DX11_UVConfig.h"
#include <d3d11.h>

class DX11_RenderSys : public RenderSys{
public:
	DX11_RenderSys(API_TYPE type, SmartPTR<ID3D11Device>& dev,
		SmartPTR<ID3D11DeviceContext>& devCtx,
		SmartPTR<IDXGISwapChain>& swapChain) :
		RenderSys(type),
		m_dev(dev), m_devCtx(devCtx), m_swapChain(swapChain) {}
public:
	// Ultility
	virtual void ResetAll() = 0;
	virtual void ClearAllBindingState() = 0;
	// For Shader Resource View
	virtual bool BindTexture() = 0; // tx
	virtual bool UnbindTexture() = 0;

	virtual bool BindUnorderAccessData() = 0; // ux
	virtual bool UnbindUnorderAccessData() = 0;

	virtual bool BindSampler() = 0; // sx
	virtual bool UnbindSampler() = 0;

	// For Shaders
	virtual bool BindShader(Shader&) = 0;
	virtual bool UnbindShader(Shader&) = 0;

	// For Pipeline State
	virtual bool BindBuffer(Buffer&) = 0;
	virtual bool UnbindBuffer(Buffer&) = 0;

	virtual bool BindRenderTarget() = 0;
	virtual bool UnbindRenderTarget() = 0;

	virtual bool SetInputLayout() = 0;
	virtual bool SetCullMode() = 0;
	virtual bool SetPrimitiveType() = 0;
	// Draw Call
	virtual void DrawIndex() = 0;
	virtual void Draw() = 0;

private:
	SmartPTR<ID3D11Device>				m_dev;
	SmartPTR<ID3D11DeviceContext> m_devCtx;
	SmartPTR<IDXGISwapChain>		m_swapChain;
};