#include "DXRenderer.h"
#include "UI.h"
#include "UnknownObject.h"
#include "Texture.h"
#include "RasterState.h"

#include "InfoLog.h"
#include "Resource.h"
#include "Texture.h"
#include "Pass.h"

#include <fstream>

D3D11_INPUT_ELEMENT_DESC DXRenderer::Static_InputElementDesc[] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

D3D11_INPUT_ELEMENT_DESC DXRenderer::Dynamic_InputElementDesc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

DXRenderer::DXRenderer() {
	// Do Nothing!
}

void DXRenderer::EndRender()
{
	m_swapChain->Present(0, 0);
}

DXRenderer& DXRenderer::GetInstance() {
	static DXRenderer _instance;
	return _instance;
}

ID3D11Device* DXRenderer::GetDevice() {
	return m_dev.Get();
}

ID3D11DeviceContext* DXRenderer::GetContext() {
	return m_devContext.Get();
}

RenderTargetWrapper* DXRenderer::GetMainRT() {
	return m_mainRenderTarget.get();
}

DepthTexture* DXRenderer::GetMainDS() {
	return m_mainDepthTexture.get();
}

// 初始化系统需要的三个关键对象(Device, DeviceContext, SwapChain)
// 以及构成渲染管线的基础的几个资源
// backbuffer以及rasterState

// 返回: 任意一个对象或者资源创建失败则返回false，否则返回true
// 输入: 
// hwnd: 与DX绑定的窗口句柄
// width/height: 视口的高和宽，单位px

bool DXRenderer::InitSys(HWND hwnd, float width, float height) {
	static const char* funcTag = "DXRenderer::InitSys: ";
	// Create a reference device
	if (!setReferenceDev(hwnd, width, height)) {
		MLOG(LL, funcTag, LL, "initialize failed!");
		return false;
	}
	// set backbuffer
	if (!setBackBuffer(hwnd, width, height)) {
		MLOG(LL, funcTag, LL, "initialize failed!");
		return false;
	}
	// Create texture for depth and texture and its view
	if (!setDepthStencil(hwnd, width, height)) {
		MLOG(LL, funcTag, LL, "initialize failed!");
		return false;
	}
	// Create Raster state and viewport
	if (!setRenderState(hwnd, width, height)) {
		MLOG(LL, funcTag, LL, "initialize failed!");
		return false;
	}
	setInputLayout();

	return true;
}

/////////////////////////////////
// private function
/////////////////////////////////

// set reference device
inline bool DXRenderer::setReferenceDev(HWND hwnd, float width, float height) {
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	// 不使用抗锯齿
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		creationFlags,
		feature, 2,
		D3D11_SDK_VERSION,
		&sd, m_swapChain.GetAddressOf(), m_dev.GetAddressOf(), NULL, m_devContext.GetAddressOf()))) {

		MLOG(LL, "DXRenderer::setReferenceDev: ", LE, "Initialize DirectX Failed!");
		return false;
	}
	return true;
}

// set back buffer
inline bool DXRenderer::setBackBuffer(HWND hwnd, float width, float height) {
	HRESULT hr = S_OK;
	ID3D11Texture2D* backBuffer = NULL;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	if (FAILED(hr)) {
		MLOG(LL, "DXRenderer::setBackBuffer: ", LL, "Get ", LE, "BACK BUFFER", LL, " Failed!");
		return false;
	}
	ID3D11RenderTargetView* tmpRTV;
	hr = m_dev->CreateRenderTargetView(backBuffer, 0, &tmpRTV);
	if (FAILED(hr)) {
		MLOG(LL, "DXRenderer::setBackBuffer: ", LL, "Create ", LE, "RENDER TARGET VIEW", LL, " Failed!");
		if (backBuffer) backBuffer->Release();
		return false;
	}

	IDXGISurface* ds = NULL;
	backBuffer->QueryInterface(__uuidof(IDXGISurface), reinterpret_cast<void**>(&ds));
	if (ds) {
		UIRenderer::GetInstance().Init(ds);
	}

	if (backBuffer) backBuffer->Release();
	if (ds) ds->Release();
	m_mainRenderTarget = std::make_shared<RenderTargetWrapper>(tmpRTV);
	return true;
}

// set depth and stencil buffer and its view
inline bool DXRenderer::setDepthStencil(HWND hwnd, float width, float height) {
	m_mainDepthTexture = std::make_shared<DepthTexture>(width, height);
	m_mainDepthTexture->Setup(m_dev.Get());
	return true;
}

// set Render state
bool DXRenderer::setRenderState(HWND hwnd, float width, float height) {
	m_defRasterState.Setup(m_dev.Get());

	ShadingPass::Def_RasterState.bindTarget = SBT_UNKNOWN;
	ShadingPass::Def_RasterState.slot = -1;
	ShadingPass::Def_RasterState.resPointer.object = &m_defRasterState;
	ShadingPass::Def_RasterState.resType = MST_UNKNOWN_OBJECT;


	m_defViewport.Width = width;
	m_defViewport.Height = height;
	m_defViewport.MinDepth = 0.0f;
	m_defViewport.MaxDepth = 1.0f;
	m_defViewport.TopLeftX = 0;
	m_defViewport.TopLeftY = 0;

	ShadingPass::Def_ViewPort = m_defViewport;

	return true;
}

// Set common Input Layout
void DXRenderer::setInputLayout() {
	// read byte code
	std::fstream file = std::fstream("../Debug/inputLayout.cso", std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		MLOG(LE, "DXRenderer::setInputLayout: can not open input layout shader file!");
		return;
	}
	byte* tmpCode = NULL;
	size_t size = -1;
	file.seekg(0, std::ios::end);
	size = file.tellg();
	file.seekg(0, std::ios::beg);
	tmpCode = new byte[size];
	file.read(reinterpret_cast<char*>(tmpCode), size);
	file.close();

	if (FAILED(m_dev->CreateInputLayout(Dynamic_InputElementDesc, 4, tmpCode, size, m_comInputLayout.ReleaseAndGetAddressOf()))) {
		MLOG(LE, "DXRenderer::setInputLayout: can not create input layout!");
		delete[] tmpCode;
		return;
	}
	delete[] tmpCode;
	m_devContext->IASetInputLayout(m_comInputLayout.Get());
}

inline void DXRenderer::clearRenderTargets() {
	m_devContext->OMSetRenderTargets(0, NULL, NULL);
}

// Set iterate object
//void DXRenderer::AddIterateObject(IterateObject* itobj) {
//	m_iterateList.push_back(itobj);
//}
//
//void DXRenderer::IterateFuncs() {
//	for (auto iter = m_iterateList.begin(); iter != m_iterateList.end(); ++iter) {
//		(*iter)->IterFunc(m_dev.Get(), m_devContext.Get());
//	}
//}

