#include "DX11RenderSys.h"
#include "../RenderSys/UVFactory.h"

#include <cassert>

namespace UnknownVision {
	bool UVFactory::createDX11RenderSys(std::unique_ptr<RenderSys>& rs, API_TYPE api, float width, float height) {
		rs = std::make_unique<DX11_RenderSys>(api, width, height);
		return true;
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

		return true;
	}

	bool DX11_RenderSys::BindShader(uint32_t index) {
		return false;
	}

	bool DX11_RenderSys::UnbindShader(uint32_t index) {
		return false;
	}

	bool DX11_RenderSys::BindVertexBuffer(uint32_t index) {
		return false;
	}

	bool DX11_RenderSys::BindRenderTarget(int index) {
		if (index == -1) { // 使用back buffer作为RTV

		}
		return false;
	}

	bool DX11_RenderSys::UnbindRenderTarget(int index) {
		if (index == -1) { // 使用back buffer作为RTV

		}
		return false;
	}

	bool DX11_RenderSys::SetPrimitiveType() {
		return false;
	}

	void DX11_RenderSys::DrawIndex() {
	}

	void DX11_RenderSys::Draw() {
		
	}
}
