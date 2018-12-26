#include "../RenderSys/RenderSys.h"
#include "DX11_UVConfig.h"
#include "../Utility/MainClass/MainClass.h"
#include <d3d11.h>

namespace UnknownVision {
	struct DX11InitParams {
		HWND hwnd;
		float width;
		float height;
	};
	class DX11_RenderSys : public RenderSys {
	public:
		DX11_RenderSys(API_TYPE type, float width, float height)
			: RenderSys(type, width, height) {}
	public:
		// Ultility
		virtual bool Init();
		virtual void ResetAll() { /*TODO*/ }
		virtual void ClearAllBindingState() { /*TODO*/ }

		// For Shaders
		virtual bool BindShader(uint32_t index);
		virtual bool UnbindShader(uint32_t index);

		virtual bool BindVertexBuffer(uint32_t index);

		virtual bool BindRenderTarget(int index);
		virtual bool UnbindRenderTarget(int index);

		virtual bool SetPrimitiveType();
		// Draw Call
		virtual void DrawIndex();
		virtual void Draw();

	private:
		SmartPTR<ID3D11Device>				m_dev;
		SmartPTR<ID3D11DeviceContext> m_devCtx;
		SmartPTR<IDXGISwapChain>		m_swapChain;
		MainClass										m_mainClass;
		SmartPTR<ID3D11RenderTargetView> m_backBufferRTV;
	};
}
