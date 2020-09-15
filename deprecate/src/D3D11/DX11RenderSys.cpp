#include "DX11RenderSys.h"
#include "./Resource/DX11ResMgr.h"
#include "../UVRoot.h"
#include "../Utility/WindowBase/win32/WindowWin32.h"
#include <cassert>

namespace UnknownVision {
	bool Root::createDX11Env(API_TYPE api, float width, float height) {
		std::unique_ptr<WindowWin32> win = std::make_unique<WindowWin32>("DX11 RenderSystem", width, height, false);
		if (!win->Init()) return false;
		m_window = std::unique_ptr<WindowBase>(reinterpret_cast<WindowBase*>(win.release()));
		m_renderSys = std::make_unique<DX11_RenderSys>(api, width, height);
		m_shaderMgr = std::make_unique<DX11_ShaderMgr>();
		m_bufferMgr = std::make_unique<DX11_BufferMgr>();
		m_vtxDeclMgr = std::make_unique<DX11_VertexDeclarationMgr>();
		return true;
	}

	bool DX11_RenderSys::Init(WindowBase* win) {
		HRESULT hr = S_OK;
		DXGI_SWAP_CHAIN_DESC sd; // 交换链描述
		D3D_FEATURE_LEVEL feature;
		// 根据API等级设置特性等级
		switch (API) {
		case DirectX11_0:
			feature = D3D_FEATURE_LEVEL_11_0;
			break;
		default:
			assert(false);
		}
		memset(&sd, 0, sizeof(sd)); // 初始化变量
		sd.BufferCount = 1;
		sd.BufferDesc.Width = static_cast<UINT>(m_basicWidth);
		sd.BufferDesc.Height = static_cast<UINT>(m_basicHeight);
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		// 设置交换链换页速度 WARNNING: 有可能影响帧率?
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = reinterpret_cast<WindowWin32*>(win)->hWnd();
		// 不使用抗锯齿
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		// 窗口化
		sd.Windowed = TRUE;

		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

		if (FAILED(D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			creationFlags,
			&feature, 1,
			D3D11_SDK_VERSION,
			&sd,
			m_swapChain.GetAddressOf(),
			m_dev.GetAddressOf(),
			NULL,
			m_devCtx.GetAddressOf()))) {
			assert(false);
			return false;
		}

		// 设置back buffer的render target
		ID3D11Texture2D* backBuffer = nullptr;
		hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
		assert(hr >= 0);
		hr = m_dev->CreateRenderTargetView(backBuffer, 0, m_backBufferRTV.GetAddressOf());
		assert(hr >= 0);
		backBuffer->Release();
		return true;
	}

	void DX11_RenderSys::ActiveViewPort(const ViewPortDesc& desc) {
		/** 创建并填写Viewport描述信息，并直接将其绑定到管线上 */
		D3D11_VIEWPORT vp;
		vp.Height = desc.height;
		vp.Width = desc.width;
		vp.MinDepth = desc.minDepth;
		vp.MaxDepth = desc.maxDepth;
		vp.TopLeftX = desc.topLeftX;
		vp.TopLeftY = desc.topLeftY;
		m_devCtx->RSSetViewports(1, &vp);
	}

	void DX11_RenderSys::ActiveVertexDeclaration(VertexDeclarationIdx index) {
		DX11_VertexDeclaration& vdc = static_cast<DX11_VertexDeclaration&>(
			Root::GetInstance().GetVertexDeclarationMgr().GetVertexDeclaration(index));
		m_devCtx->IASetInputLayout(vdc.GetDeclaration());
	}

	bool DX11_RenderSys::BindShader(ShaderIdx index) {
		const DX11_Shader& shader = static_cast<const DX11_Shader&>(
			Root::GetInstance().GetShaderMgr().GetShader(index));
		switch (shader.Type) {
		case ST_Vertex_Shader:
			m_devCtx->VSSetShader(shader.VertexShader(), nullptr, 0);
			break;
		case ST_Pixel_Shader:
			m_devCtx->PSSetShader(shader.PixelShader(), nullptr, 0);
			break;
		default:
			return false;
		}
		return true;
	}

	bool DX11_RenderSys::UnbindShader(ShaderType type) {
		switch (type) {
		case ST_Vertex_Shader:
			m_devCtx->VSSetShader(nullptr, nullptr, 0);
			break;
		case ST_Pixel_Shader:
			m_devCtx->PSSetShader(nullptr, nullptr, 0);
			break;
		default:
			return false;
		}
		return true;
	}

	bool DX11_RenderSys::BindVertexBuffer(BufferIdx index) {
		/** 从管理器检索缓冲对象
		 * 清空原有顶点缓冲，并设置唯一且刚索引的缓冲
		 * 更新缓冲数量以及唯一缓冲的元素大小
		 */
		m_vertexBuffer = static_cast<DX11_Buffer*>(
			&Root::GetInstance().GetBufferMgr().GetBuffer(index));
		m_curVtxBufs = { nullptr };
		m_curVtxStrides = { 0 };
		m_curNumVtxBuf = 1;
		m_curVtxBufs[0] = m_vertexBuffer->BufferPtr();
		m_curVtxStrides[0] = m_vertexBuffer->ElementSize();
		return true;
	}

	bool DX11_RenderSys::BindVertexBuffers(BufferIdx* indices, size_t numBuf) {
		if (numBuf == 0 || indices == nullptr) return false; /**< 假如没有缓冲或者缓冲索引队列为空，则直接返回 */
		/** 先申请临时的缓冲进行记录，避免出错时原有设置无法恢复 */
		std::array<ID3D11Buffer*, UV_MAX_VERTEX_BUFFER> vertexBufs = { nullptr };
		std::array<uint32_t, UV_MAX_VERTEX_BUFFER> vertexBufStrides = { 0 };
		DX11_Buffer* firstVertexBuffer = nullptr;
		DX11_BufferMgr& bufferMgr = static_cast<DX11_BufferMgr&>(
			Root::GetInstance().GetBufferMgr());
		/** 第一个缓冲单独处理，因为要记录第一个缓冲的引用，从这个缓冲中获得顶点缓冲的顶点数量 */
		firstVertexBuffer = static_cast<DX11_Buffer*>(&bufferMgr.GetBuffer(indices[0]));
		vertexBufs[0] = firstVertexBuffer->BufferPtr();
		vertexBufStrides[0] = firstVertexBuffer->ElementSize();
		/** 遍历剩下的索引，并填充缓冲队列以及Stride队列 */
		for (size_t i = 1; i < numBuf; ++i) {
			DX11_Buffer& buffer = static_cast<DX11_Buffer&>(bufferMgr.GetBuffer(indices[i]));
			vertexBufs[i] = buffer.BufferPtr();
			vertexBufStrides[i] = buffer.ElementSize();
		}
		/** 确定一切正常后，用临时队列替代原有队列完成设置的更新 */
		m_curNumVtxBuf = numBuf;
		m_curVtxBufs.swap(vertexBufs);
		m_curVtxStrides.swap(vertexBufStrides);
		m_vertexBuffer = firstVertexBuffer;
		return true;
	}

	bool DX11_RenderSys::BindConstantBuffer(BufferIdx index, PipelineStage stage, SlotIdx slot) {
		DX11_Buffer& cbuf = static_cast<DX11_Buffer&>(Root::GetInstance().GetBufferMgr().GetBuffer(index));
		ID3D11Buffer* bufPtr = cbuf.BufferPtr();
		switch (stage) {
		case PS_VertexProcess:
			m_devCtx->VSSetConstantBuffers(slot.value(), 1, &bufPtr);
			break;
		case PS_PixelProcess:
			m_devCtx->PSSetConstantBuffers(slot.value(), 1, &bufPtr);
			break;
		default:
			return false;
		}
		return true;
	}

	bool DX11_RenderSys::BindIndexBuffer(BufferIdx index) {
		m_indexBuffer = static_cast<DX11_Buffer*>(&Root::GetInstance().GetBufferMgr().GetBuffer(index));
		return true;
	}

	bool DX11_RenderSys::BindDepthStencilTarget(DepthStencilIdx index) {
		ID3D11DepthStencilView* dsv = static_cast<DX11_Texture2DMgr&>(Root::GetInstance().GetTexture2DMgr()).GetDepthStencilFromTexture(index);
		if (dsv == nullptr) return false;
		m_curDSV = dsv;
		return true;
	}

	bool DX11_RenderSys::BindRenderTarget(RenderTargetIdx index) {
		if (index == -1) { // 使用back buffer作为RTV
			m_curRTVS = { nullptr };
			m_curNumRTVS = 1;
			m_curRTVS[0] = m_backBufferRTV.Get();
		}
		else {
			ID3D11RenderTargetView* rtv = static_cast<DX11_Texture2DMgr&>(
				Root::GetInstance().GetTexture2DMgr()).GetRenderTargetFromTexture(index);
			if (rtv == nullptr) return false;
			m_curRTVS = { nullptr };
			m_curNumRTVS = 1;
			m_curRTVS[0] = rtv;
		}
		return true;
	}

	bool DX11_RenderSys::BindRenderTargets(RenderTargetIdx* indices, size_t numRenderTarget) {
		std::array<ID3D11RenderTargetView*, 8> rtvs = { nullptr };
		DX11_Texture2DMgr& t2dmgr = static_cast<DX11_Texture2DMgr&>(Root::GetInstance().GetTexture2DMgr());
		for (size_t i = 0; i < numRenderTarget; ++i) {
			rtvs[i] = t2dmgr.GetRenderTargetFromTexture(indices[i]);
			if (rtvs[i] == nullptr) return false;
		}
		m_curRTVS.swap(rtvs);
		m_curNumRTVS = numRenderTarget;
		return true;
	}

	void DX11_RenderSys::UnbindRenderTarget() {
		m_curRTVS = { nullptr };
		m_curNumRTVS = 0;
	}

	void DX11_RenderSys::ClearRenderTarget(RenderTargetIdx index) {
		static const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if (index == -1) {
			m_devCtx->ClearRenderTargetView(m_backBufferRTV.Get(), clearColor);
		}
		else {
			ID3D11RenderTargetView* rtv = static_cast<DX11_Texture2DMgr&>(
				Root::GetInstance().GetTexture2DMgr()).GetRenderTargetFromTexture(index);
			if (rtv == nullptr) return;
			m_devCtx->ClearRenderTargetView(rtv, clearColor);
		}
	}

	void DX11_RenderSys::SetPrimitiveType(Primitive pri) {
		switch (pri) {
		case PRI_TriangleList:
			m_devCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			break;
		default:
			MLOG(LW, __FUNCTION__, LL, " invalid primitive type");
		}
		return;
	}

	void DX11_RenderSys::Draw() {
		/**< 实际绑定渲染对象以及深度模板缓存 */
		m_devCtx->OMSetRenderTargets(m_curNumRTVS, m_curRTVS.data(), m_curDSV);
		/**< 实际绑定顶点缓存 */
		static std::array<uint32_t, UV_MAX_VERTEX_BUFFER> vertexBufferOffsets = { 0 };
		m_devCtx->IASetVertexBuffers(0, m_curNumVtxBuf, m_curVtxBufs.data(), m_curVtxStrides.data(), vertexBufferOffsets.data());

		if (m_indexBuffer != nullptr) { /**< 绑定了索引缓存，使用索引进行绘制 */
			m_devCtx->DrawIndexed(m_indexBuffer->NumberOfElements(), 0, 0);
		}
		else { /**< 没绑定索引缓存，逐顶点绘制 */
			m_devCtx->Draw(m_vertexBuffer->NumberOfElements(), 0);
		}
	}
}
