#include "DX11RenderSys.h"
#include "./Resource/DX11ResMgr.h"
#include "../UVFactory.h"

#include <cassert>

namespace UnknownVision {
	bool UVFactory::createDX11Env(std::unique_ptr<RenderSys>& rs, API_TYPE api, float width, float height) {
		rs = std::make_unique<DX11_RenderSys>(api, width, height);
		return rs->Init();
	}

	bool DX11_RenderSys::Init() {
		HRESULT hr = S_OK;
		m_mainClass.CreateDesktopWindow(m_basicWidth, m_basicHeight);
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
		sd.OutputWindow = m_mainClass.GetWindowHandle();
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

		// 创建各个资源管理器
		m_shaderMgr = std::make_unique<DX11_ShaderMgr>(m_dev.Get());
		m_bufMgr = std::make_unique<DX11_BufferMgr>(m_dev.Get());
		m_tex2DMgr = std::make_unique<DX11_Texture2DMgr>(m_dev.Get());

		return true;
	}

	int DX11_RenderSys::CreateInputLayout(std::vector<SubVertexAttributeLayoutDesc>& descs,
		int vertexShader) {
		SmartPTR<ID3D11InputLayout> il;
		// 从SubVertexAttributeLayout中转换为dx的格式
		std::vector<D3D11_INPUT_ELEMENT_DESC> eles(descs.size());
		for (size_t i = 0; i < descs.size(); ++i) {
			eles[i].SemanticName = descs[i].semantic;
			eles[i].SemanticIndex = descs[i].index;
			eles[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			eles[i].InstanceDataStepRate = 0;
			eles[i].InputSlot = descs[i].bufIdx;
			DXGI_FORMAT format;
			switch (descs[i].dataType) {
			case VADT_FLOAT1:
				format = DXGI_FORMAT_R32_FLOAT;
				break;
			case VADT_FLOAT2:
				format = DXGI_FORMAT_R32G32_FLOAT;
				break;
			case VADT_FLOAT3:
				format = DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case VADT_FLOAT4:
				format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			default:
				assert(false);
			}
			eles[i].Format = format;
			eles[i].AlignedByteOffset = descs[i].byteOffset;
		}

		DX11_Shader* shader = reinterpret_cast<DX11_Shader*>(&m_shaderMgr->GetShader(vertexShader));
		if (FAILED(m_dev->CreateInputLayout(eles.data(), eles.size(),
			shader->ByteCode()->GetBufferPointer(), 
			shader->ByteCode()->GetBufferSize(), il.GetAddressOf()))) {
			return -1;
		}
		m_inputLayouts.push_back(il);
		return m_inputLayouts.size() - 1;
	}

	bool DX11_RenderSys::ActiveInputLayout(uint32_t index) {
		if (index < 0 || index >= m_inputLayouts.size()) {
			return false;
		}
		m_devCtx->IASetInputLayout(m_inputLayouts[index].Get());
		return true;
	}

	int DX11_RenderSys::CreateViewPort(const ViewPortDesc& desc) {
		D3D11_VIEWPORT vp;
		vp.Height = desc.height;
		vp.Width = desc.width;
		vp.MinDepth = desc.minDepth;
		vp.MaxDepth = desc.maxDepth;
		vp.TopLeftX = desc.topLeftX;
		vp.TopLeftY = desc.topLeftY;
		m_viewports.push_back(vp);
		return m_viewports.size() - 1;
	}

	bool DX11_RenderSys::ActiveViewPort(uint32_t index) {
		assert(index < m_viewports.size());
		m_devCtx->RSSetViewports(1, &m_viewports[index]);
		return true;
	}

	bool DX11_RenderSys::BindShader(uint32_t index) {
		DX11_Shader* shader = reinterpret_cast<DX11_Shader*>(&m_shaderMgr->GetShader(index));
		switch (shader->Type) {
		case ST_Vertex_Shader:
			m_devCtx->VSSetShader(shader->VertexShader().Get(), nullptr, 0);
			break;
		case ST_Pixel_Shader:
			m_devCtx->PSSetShader(shader->PixelShader().Get(), nullptr, 0);
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

	bool DX11_RenderSys::BindVertexBuffer(uint32_t index) {
		DX11_Buffer* buffer = reinterpret_cast<DX11_Buffer*>(&m_bufMgr->GetBuffer(index));
		ID3D11Buffer* bufptr = buffer->BufferPtr();
		UINT stride = buffer->ByteSize() / buffer->NumEle();
		UINT offset = 0;
		m_devCtx->IASetVertexBuffers(0, 1, &bufptr, &stride, &offset);
		m_numVertexBufferEles = buffer->NumEle();
		return true;
	}

	bool DX11_RenderSys::BindVertexBuffers(uint32_t* indices, size_t numBuf) {
		/*TODO*/
		return false;
	}

	bool DX11_RenderSys::BindConstantBuffer(uint32_t index, PipelineStage stage, uint32_t slot) {
		DX11_Buffer* buffer = reinterpret_cast<DX11_Buffer*>(&m_bufMgr->GetBuffer(index));
		ID3D11Buffer* bufPtr = buffer->BufferPtr();
		switch (stage) {
		case PS_VertexProcess:
			m_devCtx->VSSetConstantBuffers(slot, 1, &bufPtr);
			break;
		case PS_PixelProcess:
			m_devCtx->PSSetConstantBuffers(slot, 1, &bufPtr);
			break;
		}
		return true;
	}

	bool DX11_RenderSys::BindDepthStencilTarget(uint32_t index) {
		DX11_Texture2D* tex2d = reinterpret_cast<DX11_Texture2D*>(&m_tex2DMgr->GetTexture(index));
		m_curDepthStencilView = tex2d->DepthStencilView();
		return (m_curDepthStencilView != nullptr);
	}

	bool DX11_RenderSys::BindRenderTarget(int index) {
		if (index == -1) { // 使用back buffer作为RTV
			m_devCtx->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_curDepthStencilView);
		}
		else {
			DX11_Texture2D* tex2d = reinterpret_cast<DX11_Texture2D*>(&m_tex2DMgr->GetTexture(index));
			ID3D11RenderTargetView* rtv = tex2d->RenderTargetView();
			if (rtv == nullptr) return false;
			m_devCtx->OMSetRenderTargets(1, &rtv, m_curDepthStencilView);
		}
		return true;
	}

	void DX11_RenderSys::UnbindRenderTarget() {
		m_devCtx->OMSetRenderTargets(0, nullptr, nullptr);
	}

	void DX11_RenderSys::ClearRenderTarget(int index) {
		static const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if (index == -1) {
			m_devCtx->ClearRenderTargetView(m_backBufferRTV.Get(), clearColor);
		}
		else {
			DX11_Texture2D* tex2d = reinterpret_cast<DX11_Texture2D*>(&m_tex2DMgr->GetTexture(index));
			ID3D11RenderTargetView* rtv = tex2d->RenderTargetView();
			assert(rtv != nullptr);
			m_devCtx->ClearRenderTargetView(rtv, clearColor);
		}
	}

	bool DX11_RenderSys::SetPrimitiveType(Primitive pri) {
		switch (pri) {
		case PRI_Triangle:
			m_devCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			break;
		default:
			return false;
		}
		return true;
	}

	void DX11_RenderSys::DrawIndex() {
		/* TODO */
	}

	void DX11_RenderSys::Draw() {
		m_devCtx->Draw(m_numVertexBufferEles, 0);
	}
}
