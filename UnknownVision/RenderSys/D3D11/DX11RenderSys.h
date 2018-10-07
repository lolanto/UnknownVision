#include "../RenderSys.h"
#include "DX11_UVConfig.h"
#include <d3d11.h>

class DX11_RenderSys : public RenderSys{
public:
	DX11_RenderSys(API_TYPE type, SmartPTR<ID3D11Device>& dev,
		SmartPTR<ID3D11DeviceContext>& devCtx,
		SmartPTR<IDXGISwapChain>& swapChain) :
		RenderSys(type),
		m_dev(dev), m_devCtx(devCtx), m_swapChain(swapChain) {}
private:
	SmartPTR<ID3D11Device>				m_dev;
	SmartPTR<ID3D11DeviceContext> m_devCtx;
	SmartPTR<IDXGISwapChain>		m_swapChain;
};