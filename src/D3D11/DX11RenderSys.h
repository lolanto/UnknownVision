#ifndef D3D11_RENDER_SYS_H
#define D3D11_RENDER_SYS_H
#include "../RenderSys/RenderSys.h"
#include "DX11_UVConfig.h"
#include <d3d11.h>
#include <array>

namespace UnknownVision {
	class DX11_Buffer;
	class DX11_RenderSys : public RenderSys {
	public:
		DX11_RenderSys(API_TYPE type, float width, float height)
			: RenderSys(type, width, height) {}
	public:
		// Ultility Functions
		/// 初始化渲染系统，初始化过程同时负责渲染主窗口的创建
		/** 该函数主要完成几个任务，
		 * 1. 创建渲染使用的主窗口，窗口的特性需要满足渲染系统需要的特性
		 * 2. 创建渲染系统需要的几个基本对象，在DX11中主要是Device, Swapchain
		 * 3. DX11中还需要创建与SwapChain密切相关的BackBuffer
		 * @return 初始化成功返回True，失败返回False
		 */
		virtual bool Init(WindowBase* win);
		virtual void ResetAll() { /*TODO*/ }

		/// For Shaders
		virtual bool BindShader(ShaderIdx index); /**< 继承自RenderSys */
		virtual bool UnbindShader(ShaderType type); /**< 继承自RenderSys */

		virtual void ActiveVertexDeclaration(VertexDeclarationIdx index); /**< 继承自RenderSys */

		virtual void ActiveViewPort(const ViewPortDesc& desc); /**< 继承自RenderSys */

		virtual bool BindVertexBuffer(BufferIdx index); /**< 继承自RenderSys */
		virtual bool BindVertexBuffers(BufferIdx* indices, size_t numBuf); /**< 继承自RenderSys */
		virtual bool BindConstantBuffer(BufferIdx index, PipelineStage stage, SlotIdx slot); /**< 继承自RenderSys */
		virtual bool BindIndexBuffer(BufferIdx index); /**< 继承自RenderSys */
		virtual void UnbindIndexBuffer() { m_indexBuffer = nullptr; }

		virtual bool BindDepthStencilTarget(DepthStencilIdx index); /**< 继承自RenderSys */
		virtual void UnbindDepthStencilTarget() { m_curDSV = nullptr; } /**< 继承自RenderSys */
		virtual bool BindRenderTarget(RenderTargetIdx index); /**< 继承自RenderSys */
		virtual bool BindRenderTargets(RenderTargetIdx* indices, size_t numRenderTarget); /**< 继承自RenderSys */
		virtual void UnbindRenderTarget(); /**< 继承自RenderSys */
		virtual void ClearRenderTarget(RenderTargetIdx index); /**< 继承自RenderSys */

		virtual void SetPrimitiveType(Primitive pri); /**< 继承自RenderSys */
		/// Draw Call
		virtual void Draw(); /**< 继承自RenderSys */
		virtual void Present() { m_swapChain->Present(0, 0); } /**< 继承自RenderSys */

		/** DX11特殊的实现，返回DX11设备供
		 * DX11相关的Mgr实现资源创建功能
		 */
		ID3D11Device* GetDevice() { return m_dev.Get(); }

	private:
		SmartPTR<ID3D11Device>				m_dev; /**< DX11设备 */
		SmartPTR<ID3D11DeviceContext> m_devCtx; /**< DX11运行环境 */
		SmartPTR<IDXGISwapChain>		m_swapChain; /**< DX11交换链 */
		SmartPTR<ID3D11RenderTargetView> m_backBufferRTV; /**< 与当前交换链绑定的backbuffer的RenderTarget */
		/// 由于DX要求RenderTarget和DepthStencil同时设置，故需要临时存储这两个对象，等draw时再统一绑定
		ID3D11DepthStencilView* m_curDSV; /**< 当前绑定的深度模板测试缓冲 */
		size_t m_curNumRTVS = 0; /**< m_curRTVS中设置的渲染对象的数量 */
		std::array<ID3D11RenderTargetView*, UV_MAX_RENDER_TARGET> m_curRTVS; /**< 当前绑定的渲染对象缓冲队列 */
		
		size_t m_curNumVtxBuf = 0; /**< 当前绑定的顶点缓冲的数量 */
		std::array<ID3D11Buffer*, UV_MAX_VERTEX_BUFFER> m_curVtxBufs; /**< 当前绑定的顶点缓冲的对象队列 */
		std::array<uint32_t, UV_MAX_VERTEX_BUFFER> m_curVtxStrides; /**< 对应当前绑定的每个顶点缓冲中顶点数据的大小 */
		DX11_Buffer* m_vertexBuffer = nullptr; /**< m_curVtxBufs[0]的缓冲区 */
		DX11_Buffer* m_indexBuffer = nullptr; /**< 当前可能绑定的索引缓冲 */
	};
}
#endif // D3D11_RENDER_SYS_H
