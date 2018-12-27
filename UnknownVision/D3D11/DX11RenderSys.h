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
		virtual bool UnbindShader(ShaderType type);

		virtual int CreateInputLayout(std::vector<SubVertexAttributeLayoutDesc>& descs,
			int vertexShader);
		virtual bool ActiveInputLayout(uint32_t index);

		virtual bool BindVertexBuffer(uint32_t index);
		virtual bool BindVertexBuffers(uint32_t* indices, size_t numBuf);
		virtual bool BindConstantBuffer(uint32_t index, PipelineStage stage, uint32_t slot);

		virtual bool BindDepthStencilTarget(uint32_t index);
		virtual void UnbindDepthStencilTarget() { m_curDepthStencilView = nullptr; }
		virtual bool BindRenderTarget(int index);
		virtual void UnbindRenderTarget();

		virtual bool SetPrimitiveType(Primitive pri);
		// Draw Call
		virtual void DrawIndex();
		virtual void Draw();
		virtual void Present() { m_swapChain->Present(0, 0); }

	private:
		SmartPTR<ID3D11Device>				m_dev;
		SmartPTR<ID3D11DeviceContext> m_devCtx;
		SmartPTR<IDXGISwapChain>		m_swapChain;
		SmartPTR<ID3D11RenderTargetView> m_backBufferRTV;
		ID3D11DepthStencilView*				m_curDepthStencilView = nullptr;
		// 当前绑定的vertex buffer包含的顶点数量
		uint32_t										m_numVertexBufferEles = 0;
		MainClass										m_mainClass;
		std::vector<SmartPTR<ID3D11InputLayout>> m_inputLayouts;
	};
}
