#include "DXRenderer.h"
#include "MeshManager.h"
#include "Model.h"
#include "Camera.h"
#include "Canvas.h"
#include "Light.h"
#include "Shader.h"
#include "UI.h"
#include "UnknownObject.h"
#include "Texture.h"
#include "RasterState.h"

#include "InfoLog.h"

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

void DXRenderer::SetRenderState(RasterState* rs, D3D11_VIEWPORT* viewport)
{	
	// Set Raster state
	if (viewport) m_devContext->RSSetViewports(1, viewport);
	else m_devContext->RSSetViewports(1, &m_viewport);
	//m_devContext->RSSetState(m_rasterState.Get());
	rs->Bind(m_devContext.Get());
}

void DXRenderer::EndRender()
{
	m_swapChain->Present(0, 0);
}

DXRenderer& DXRenderer::GetInstance() {
	static DXRenderer _instance;
	return _instance;
}

ID3D11Device** DXRenderer::GetDevice() {
	return m_dev.GetAddressOf();
}

ID3D11DeviceContext** DXRenderer::GetDeviceContext() {
	return m_devContext.GetAddressOf();
}

IRenderTarget* DXRenderer::GetMainRT() {
	return &m_mainRenderTarget;
}

ID3D11DepthStencilView* DXRenderer::GetMainDepthStencilBuffer() {
	return m_depthStencilView.Get();
}

ITexture* DXRenderer::GetMainDS() {
	return m_mainDSResource.get();
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

void DXRenderer::Setup(UObject* o) {
	o->Setup(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Setup(Canvas* c) {
	c->Setup(m_dev.Get());
}

void DXRenderer::Setup(ITexture* ct) {
	ct->Setup(m_dev.Get());
}

void DXRenderer::Setup(LightProxy* l) {
	l->Setup(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Setup(IShader* s) {
	s->Setup(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Setup(UnknownObject* uo) {
	uo->Setup(m_dev.Get());
}

void DXRenderer::Bind(UObject* o) {
	o->Bind(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Bind(RenderTargetsDesc* desc, bool enableDepthStencil, bool clearDepthStencil) {
	if (!desc->data) {
		// initialize the render target array
		ID3D11RenderTargetView** tmpViews = new ID3D11RenderTargetView*[desc->num];
		for (UINT i = 0; i < desc->num; ++i) {
			tmpViews[i] = desc->targets[i]->GetRTV();
		}
		desc->data.reset(tmpViews);
	}
	static UINT count = -1;
	static float clearColor[4] = { 0.2f, 0.4f, 0.6f, 0.0f };
	for (count = 0; count < desc->num; ++count) {
		// set bind state
		ID3D11RenderTargetView* tmp = desc->targets[count]->GetRTV();
		if (desc->clear[count]) m_devContext->ClearRenderTargetView(tmp, clearColor);
	}
	if (enableDepthStencil && clearDepthStencil) {
		m_devContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
	if (enableDepthStencil)
		m_devContext->OMSetRenderTargets(desc->num, desc->data.get(), m_depthStencilView.Get());
	else
		m_devContext->OMSetRenderTargets(desc->num, desc->data.get(), NULL);
}

void DXRenderer::Bind(ShaderResourcesDesc* desc) {
	static UINT count = -1;
	for (UINT count = 0; count < desc->num; ++count) {
		switch (desc->targets[count]) {
		case SRVT_VERTEX_SHADER:
			m_devContext->VSSetShaderResources(desc->slots[count], 1, desc->resources[count]->GetSRV());
			break;
		case SRVT_PIXEL_SHADER:
			m_devContext->PSSetShaderResources(desc->slots[count], 1, desc->resources[count]->GetSRV());
			break;
		default:
			MLOG(LE, "DXRenderer::Bind: invalid shader resource target!");
			assert(0);
		}
	}
}

void DXRenderer::Bind(LightProxy* l) {
	l->Bind(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Bind(IShader* s) {
	s->Bind(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Bind(UnknownObject* uo) {
	uo->Bind(m_devContext.Get());
}

void DXRenderer::Unbind(UObject* o) {
	o->Unbind(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Unbind(RenderTargetsDesc* desc) {
	static UINT count = -1;
	for (count = 0; count < desc->num; ++count) {
		// set unbind state
		//desc->targets[count]->UnbindRTV();
	}
	clearRenderTargets();
}

void DXRenderer::Unbind(ShaderResourcesDesc* desc) {
	static UINT count = -1;
	static ID3D11ShaderResourceView* nullArray[] = { NULL };
	for (UINT count = 0; count < desc->num; ++count) {
		switch (desc->targets[count]) {
		case SRVT_VERTEX_SHADER:
			m_devContext->VSSetShaderResources(desc->slots[count], 1, nullArray);
			break;
		case SRVT_PIXEL_SHADER:
			m_devContext->PSSetShaderResources(desc->slots[count], 1, nullArray);
			break;
		default:
			MLOG(LE, "DXRenderer::Unbind: invalid shader resource target !");
			assert(0);
		}
		// set unbind state
		//desc->resources[count]->UnbindSRV();
	}
}

void DXRenderer::Unbind(LightProxy* l) {
	l->Unbind(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Unbind(IShader* s) {
	s->Unbind(m_dev.Get(), m_devContext.Get());
}

void DXRenderer::Unbind(UnknownObject* uo) {
	uo->Unbind(m_devContext.Get());
}

void DXRenderer::ClearRenderTarget(IRenderTarget* irt) {
	static float clearColor[] = { 0.2, 0.4, 0.6, 0.0 };
	m_devContext->ClearRenderTargetView(irt->GetRTV(), clearColor);
}

void DXRenderer::ClearDepthStencil(ID3D11DepthStencilView* dsv) {
	m_devContext->ClearDepthStencilView(dsv,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
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
	m_mainRenderTarget = CommonRenderTarget(tmpRTV);
	return true;
}

// set depth and stencil buffer and its view
inline bool DXRenderer::setDepthStencil(HWND hwnd, float width, float height) {
	D3D11_TEXTURE2D_DESC depthTexDesc;
	ZeroMemory(&depthTexDesc, sizeof(depthTexDesc));
	depthTexDesc.Width = width;
	depthTexDesc.Height = height;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	//depthTexDesc.ArraySize = 6;
	depthTexDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthTexDesc.SampleDesc.Count = 1;
	depthTexDesc.SampleDesc.Quality = 0;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;

	if (FAILED(m_dev->CreateTexture2D(&depthTexDesc, 0, 
		m_depthStencilTexture.ReleaseAndGetAddressOf()))) {
		MLOG(LL, "DXRenderer::setDepthStencil: ", LL, "Create ", 
			LE, "DEPTH AND STENCIL TEXTURE", LL, " Failed!");
		return false;
	}
	D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
	ZeroMemory(&dsViewDesc, sizeof(dsViewDesc));
	dsViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	//dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	dsViewDesc.Texture2D.MipSlice = 0;
	//dsViewDesc.Texture2DArray.ArraySize = 6;
	//dsViewDesc.Texture2DArray.FirstArraySlice = 0;
	//dsViewDesc.Texture2DArray.MipSlice = 0;

	if (FAILED(m_dev->CreateDepthStencilView(m_depthStencilTexture.Get(), &dsViewDesc, 
		m_depthStencilView.ReleaseAndGetAddressOf()))) {
		MLOG(LL, "DXRenderer::setDepthStencil: ", LL, "Create ", 
			LE, "DEPTH AND STENCIL VIEW", LL, " Failed!");
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	srvd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvd.Texture2D.MipLevels = 1;
	srvd.Texture2D.MostDetailedMip = 0;
	//srvd.Texture2DArray.ArraySize = 6;
	//srvd.Texture2DArray.FirstArraySlice = 0;
	//srvd.Texture2DArray.MipLevels = 1;
	//srvd.Texture2DArray.MostDetailedMip = 0;

	ID3D11ShaderResourceView* tmpRTV;
	if (FAILED(m_dev->CreateShaderResourceView(m_depthStencilTexture.Get(), &srvd, &tmpRTV))) {
		MLOG(LL, "DXRenderer::setDepthStencil: ", LE, "Create depth and stencil shader resource failed!");
		return false;
	}
	m_mainDSResource = std::make_shared<InternalTexture>(tmpRTV);

	return true;
}

// set Render state
bool DXRenderer::setRenderState(HWND hwnd, float width, float height) {
	//// Take attention to the Front Counter Clock Wise
	//D3D11_RASTERIZER_DESC rasterDesc;
	//ZeroMemory(&rasterDesc, sizeof(rasterDesc));
	//rasterDesc.FillMode = D3D11_FILL_SOLID;
	//rasterDesc.CullMode = D3D11_CULL_BACK;
	//// 定义正面为顺时针绕序
	//rasterDesc.FrontCounterClockwise = FALSE;
	//rasterDesc.DepthBias = 0;
	//rasterDesc.SlopeScaledDepthBias = 0.0f;
	//rasterDesc.DepthBiasClamp = 0.0f;
	//rasterDesc.DepthClipEnable = TRUE;
	//rasterDesc.ScissorEnable = FALSE;
	//rasterDesc.MultisampleEnable = FALSE;
	//rasterDesc.AntialiasedLineEnable = FALSE;

	//if (FAILED(m_dev->CreateRasterizerState(&rasterDesc, m_rasterState.ReleaseAndGetAddressOf()))) {
	//	MLOG(LL, "DXRenderer::setRenderState: ", LL, "Create ", LE, "RASTER STATE", LL, " Failed!");
	//	return false;
	//}

	HRESULT hr;
	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

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
void DXRenderer::AddIterateObject(IterateObject* itobj) {
	m_iterateList.push_back(itobj);
}

void DXRenderer::IterateFuncs() {
	for (auto iter = m_iterateList.begin(); iter != m_iterateList.end(); ++iter) {
		(*iter)->IterFunc(m_dev.Get(), m_devContext.Get());
	}
}

//bool DXRenderer::BindModel(Model * m)
//{
//	static const char* funcTag = "DXRenderer::BindModel: ";
//	// 创建shader common buffer，主要是model的transform矩阵
//	const void* data = NULL;
//	size_t size = 0;
//	HRESULT hr;
//	Microsoft::WRL::ComPtr<ID3D11Buffer> tempBuf;
//	m->GetModelData(data, size);
//	if (!data) {
//		MLOG(LL, funcTag, LL, "can not get model data");
//		return false;
//	}
//	D3D11_BUFFER_DESC bufDesc;
//	D3D11_SUBRESOURCE_DATA initData;
//	ZeroMemory(&bufDesc, sizeof(bufDesc));
//	bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	bufDesc.ByteWidth = size;
//	bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//	bufDesc.MiscFlags = 0;
//	bufDesc.StructureByteStride = 0;
//	bufDesc.Usage = D3D11_USAGE_DYNAMIC;
//
//	initData.pSysMem = data;
//	initData.SysMemPitch = 0;
//	initData.SysMemSlicePitch = 0;
//
//	hr = m_dev->CreateBuffer(&bufDesc, &initData, tempBuf.ReleaseAndGetAddressOf());
//	if (FAILED(hr)) {
//		MLOG(LL, funcTag, LL, "create buffer failed!");
//		return false;
//	}
//	
//	auto iter = m_bufferList.insert(std::make_pair(m_curToken, tempBuf));
//	if (!iter.second) {
//		MLOG(LL, funcTag, LL, "insert buffer failed!");
//		return false;
//	}
//	m->SetToken(m_curToken);
//
//	calcNextToken();
//
//	return true;
//}
//
//void DXRenderer::SetupModel(Model* m) {
//	static const char* funcTag = "DXRenderer::SetupModel: ";
//	auto iter = m_bufferList.find(m->GetToken());
//	if (iter == m_bufferList.end()) {
//		// 找不到该model对应的buffer
//		MLOG(LW, funcTag, LL, "there is no curresponding model data!");
//		return;
//	}
//	if (m->isDirty()) {
//		// 需要更新该缓冲区
//		const void* data = NULL;
//		size_t size = 0;
//		m->GetModelData(data, size);
//		if (!data) {
//			// 找不到数据
//			MLOG(LW, funcTag, LL, "can not get the new model data!");
//			return;
//		}
//		D3D11_MAPPED_SUBRESOURCE subresource;
//		m_devContext->Map(iter->second.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
//		memcpy_s(subresource.pData, size, data, size);
//		m_devContext->Unmap(iter->second.Get(), 0);
//	}
//	m_devContext->VSSetConstantBuffers(1, 1, iter->second.GetAddressOf());
//}

//bool DXRenderer::BindCanvas(Canvas* c) {
//	static const char* funcTag = "DXRenderer::BindCanvas: ";
//	HRESULT hr;
//	D3D11_TEXTURE2D_DESC desc;
//	D3D11_RENDER_TARGET_VIEW_DESC rtViewDesc;
//	D3D11_SHADER_RESOURCE_VIEW_DESC srViewDesc;
//	ZeroMemory(&desc, sizeof(desc));
//	ZeroMemory(&rtViewDesc, sizeof(rtViewDesc));
//	ZeroMemory(&srViewDesc, sizeof(srViewDesc));
//	// Create texture2d
//	desc.ArraySize = 1;
//	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
//	desc.CPUAccessFlags = 0;
//	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
//	desc.Height = c->m_height;
//	desc.Width = c->m_width;
//	desc.MipLevels = 0;
//	desc.MiscFlags = 0;
//	desc.SampleDesc.Count = 1;
//	desc.SampleDesc.Quality = 0;
//	desc.Usage = D3D11_USAGE_DEFAULT;
//
//	Microsoft::WRL::ComPtr<ID3D11Texture2D> tmpTex2d;
//	hr = m_dev->CreateTexture2D(&desc, NULL, tmpTex2d.ReleaseAndGetAddressOf());
//	if (FAILED(hr)) {
//		MLOG(LL, funcTag, LL, "create texture2d failed!");
//		return false;
//	}
//
//	// create render target view
//	rtViewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
//	rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
//	rtViewDesc.Texture2D.MipSlice = 0;
//
//	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> tmpRTV;
//	hr = m_dev->CreateRenderTargetView(tmpTex2d.Get(), &rtViewDesc, tmpRTV.ReleaseAndGetAddressOf());
//	if (FAILED(hr)) {
//		MLOG(LL, funcTag, LL, "create render target view falied!");
//		return false;
//	}
//	
//	// create shader resource view
//	srViewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
//	srViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
//	srViewDesc.Texture2D.MipLevels = 1;
//	srViewDesc.Texture2D.MostDetailedMip = 0;
//
//	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tmpSRV;
//	hr = m_dev->CreateShaderResourceView(tmpTex2d.Get(), &srViewDesc, tmpSRV.ReleaseAndGetAddressOf());
//	if (FAILED(hr)) {
//		MLOG(LL, funcTag, LL, "create shader resrouce failed!");
//		return false;
//	}
//
//	c->SetTexture2D(tmpTex2d);
//	c->SetRTView(tmpRTV);
//	c->SetSRView(tmpSRV);
//
//	return true;
//}
//
//void DXRenderer::SetupCanvasAsRT(Canvas* c, bool isClear) {
//	static float clearColor[] = { 0.2f, 0.4f, 0.6f, 0.0 };
//	// bind render target
//	m_devContext->OMSetRenderTargets(1, c->GetRTV(), m_depthStencilView.Get());
//	// clear RT
//	if (isClear) m_devContext->ClearRenderTargetView(*(c->GetRTV()), clearColor);
//	// clear depth stencil
//	m_devContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
//}
//
//void DXRenderer::SetupCanvasAsSR(Canvas* c, int slot) {
//	// bind canvas to pixel shader and responded slot
//	m_devContext->PSSetShaderResources(slot, 1, c->GetSRV());
//}
//
//void DXRenderer::SetdownCanvasAsSR(int slot) {
//	static ID3D11ShaderResourceView* tmp[] = { NULL };
//	m_devContext->PSSetShaderResources(slot, 1, tmp);
//}
