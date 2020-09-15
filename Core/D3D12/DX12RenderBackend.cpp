#include "DX12RenderBackend.h"
#include "DX12RenderDevice.h"
#include "DX12Shader.h"
#include "../../Utility/InfoLog/InfoLog.h"
#include <list>

#define XifFailed(function, behavior) if (FAILED(function)) behavior
BEG_NAME_SPACE
#ifdef API_TYPE == DX12

DX12RenderBackend* DX12RenderBackend::Get() {
	static DX12RenderBackend _singleton;
	return &_singleton;
}

RenderBackend* RenderBackend::Get() {
	return DX12RenderBackend::Get();
}

DX12RenderBackend::~DX12RenderBackend()
{
	for (auto devPtr : m_devices) {
		if (devPtr) delete devPtr;
	}
	if (m_factory) m_factory.Reset();
}

bool DX12RenderBackend::Initialize()
{
	if (m_isInitialized) return true;
	HRESULT hr = S_OK;
	/** 创建DXGI工厂!  该过程可用于激活Debug Layer */
	UINT factoryFlag = 0;
#ifdef _DEBUG
	SmartPTR<ID3D12Debug3> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif // DEBUG
	if (FAILED(CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&m_factory)))) {
		LOG_ERROR("Create DXGI factory FAILED!");
		return false;
	}

	return m_isInitialized = true;
}

RenderDevice * DX12RenderBackend::CreateDevice(void * parameters)
{
	if (!m_isInitialized) return nullptr;
	SmartPTR<ID3D12Device> device;
	SmartPTR<ID3D12CommandQueue> cmdQueue;
	SmartPTR<IDXGISwapChain1> swapChain;
	DX12BackendUsedData datas = *reinterpret_cast<DX12BackendUsedData*>(parameters);
	HWND winHwnd = reinterpret_cast<HWND>(std::get<0>(datas));
	uint32_t width = std::get<1>(datas);
	uint32_t height = std::get<2>(datas);
	/** 枚举设备，创建device */
	SmartPTR<IDXGIAdapter4> adapter;
	{
		for (UINT adapterIdx = 0; m_factory->EnumAdapters(adapterIdx,
			reinterpret_cast<IDXGIAdapter**>(adapter.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND; ++adapterIdx) {
			DXGI_ADAPTER_DESC3 adapterDesc;
			adapter->GetDesc3(&adapterDesc);
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; /**< 不使用软件模拟的设备 */
			/** 检查该设备是否支持dx12 */
			XifFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)), {
				return nullptr;
				});
			break;
		}
	}
	/** 创建关键指令提交队列，只有该队列能够操作交换链进行present */
	{
		D3D12_COMMAND_QUEUE_DESC queDesc;
		queDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queDesc.NodeMask = 0; /**< 不使用多设备 */
		queDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; /**< CommandQueue需要分配优先级 */
		XifFailed(device->CreateCommandQueue(&queDesc, IID_PPV_ARGS(&cmdQueue)), {
			return nullptr;
			});
	}
	/** 创建交换链 */
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; /**< 指定交换链缓冲的透明行为，暂且不进行特殊设置 */
		swapChainDesc.BufferCount = NUMBER_OF_BACK_BUFFERS; /**< 指定后台缓冲的数量 */
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; /**< 交换链缓冲的用途，默认已有backbuffer */
		swapChainDesc.Flags = 0; /**< 不进行特殊设置 */
		swapChainDesc.Format = ElementFormatToDXGIFormat(BackBufferFormat);
		swapChainDesc.Height = height;
		swapChainDesc.Width = width;
		/** 不进行多重采样 */
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE; /**< 当窗口被拉伸时不进行适应性操作 */
		swapChainDesc.Stereo = false;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		
		XifFailed(m_factory->CreateSwapChainForHwnd(cmdQueue.Get(),
			winHwnd, &swapChainDesc,
			nullptr,  /**< 不使用全屏，可直接传入null*/
			nullptr, /**< 暂时不对输出做任何处理*/
			swapChain.GetAddressOf()), {
				return nullptr;
			});
	}
	SmartPTR<IDXGISwapChain3> sc3;
	swapChain.As(&sc3);
	m_devices.push_back(new DX12RenderDevice(*this, cmdQueue, sc3, device, adapter, width, height, 0)); /**< TODO: 暂时不支持多设备,node mask默认为0 */
	return m_devices.back();
}

RenderDevice* DX12RenderBackend::GetDevice(size_t node) {
	return m_devices[node];
}

bool DX12RenderBackend::InitializeShaderObject(BasicShader* shader) {
	/** 为shader分配shader handle */
	if (shader->m_filePath.empty() == false) {
		shader->m_handle = GShaderManager.Compile(shader->m_filePath, shader->GetShaderType());
	}
	else if (shader->m_srcCode) {
		shader->m_handle = GShaderManager.Compile(shader->m_srcCode, shader->GetShaderType(), shader->Name());
	}
	else { return false; }
	return true;
}

#endif // API_TYPE == DX12
END_NAME_SPACE
