#include "UVFactory.h"
#include "D3D11/DX11RenderSys.h"
#include "../Utility/InfoLog/InfoLog.h"

UVFactory& UVFactory::GetInstance() {
	static UVFactory _instance;
	return _instance;
}

bool UVFactory::Init(API_TYPE apiType, MainClass& mc) {
	if (m_hasInit) {
		MLOG(LL, __FUNCTION__, LL, "factory has initialize");
		return true;
	}
	m_apiType = apiType;
	switch (apiType) {
	case DirectX11_0:
		if (!createDX11RenderSys(mc.GetWindowHandle(),
			mc.GetWindowWidth(),
			mc.GetWindowHeight())) {
			MLOG(LW, __FUNCTION__, LL, "create Render System failed!");
			return false;
		}
		break;
	default:
		MLOG(LW, __FUNCTION__, LL, "unknown api type");
		return false;
	}
	m_hasInit = true;
	return true;
}

bool UVFactory::createDX11RenderSys(
	HWND hwnd, float winWidth, float winHeight) {
	SmartPTR<IDXGISwapChain> swapChain;
	SmartPTR<ID3D11Device> dev;
	SmartPTR<ID3D11DeviceContext> devCtx;
	DXGI_SWAP_CHAIN_DESC sd;
	D3D_FEATURE_LEVEL feature;

	switch (m_apiType) {
	case DirectX11_0:
		feature = D3D_FEATURE_LEVEL_11_0;
		break;
	}

	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = winWidth;
	sd.BufferDesc.Height = winHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	// ≤ª π”√øπæ‚≥›
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		creationFlags,
		&feature, 1,
		D3D11_SDK_VERSION,
		&sd, swapChain.GetAddressOf(), dev.GetAddressOf(), NULL, devCtx.GetAddressOf()))) {
		MLOG(LE, __FUNCTION__, LL, "create device and swap chain failed!");
		return false;
	}

	try {
		// Create DX11 Render System
		m_renderSys = std::make_shared<DX11_RenderSys>(m_apiType, dev, devCtx, swapChain);
		// Create Resource Manager
	}
	catch (const std::bad_alloc& e) {
		MLOG(LE, __FUNCTION__, LL, "initialize DX11 Render System failed!");
		return false;
	}

	return true;
}