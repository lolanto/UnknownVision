#include "DX12RenderBasic.h"
#include "../Utility/WindowBase/win32/WindowWin32.h"
#include "../Utility/InfoLog/InfoLog.h"

#define XifFailed(function, behavior) if (FAILED(function)) behavior

namespace UnknownVision {
	bool UnknownVision::DX12RenderBackend::Initialize()
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
			MLOG("Create DXGI factory FAILED!");
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
		BackendUsedData datas = *reinterpret_cast<BackendUsedData*>(parameters);
		const WindowWin32* win = std::get<0>(datas);
		uint32_t width = std::get<1>(datas);
		uint32_t height = std::get<2>(datas);
		/** 枚举设备，创建device */
		{
			SmartPTR<IDXGIAdapter4> adapter;
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
		/** 创建关键指令提交队列 */
		{
			D3D12_COMMAND_QUEUE_DESC queDesc;
			queDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queDesc.NodeMask = 0; /**< 不使用多设备 */
			queDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH; /**< CommandQueue需要分配优先级，这里测试使用高优先级 */
			XifFailed(device->CreateCommandQueue(&queDesc, IID_PPV_ARGS(&cmdQueue)), {
				return nullptr;
			});
		}
		/** 创建交换链 */
		{
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; /**< 指定交换链缓冲的透明行为，暂且不进行特殊设置 */
			swapChainDesc.BufferCount = NUMBER_OF_BACK_BUFFERS; /**< 指定后台缓冲的数量 */
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; /**< 交换链缓冲的用途，默认已有backbuffer */
			swapChainDesc.Flags = 0; /**< 不进行特殊设置 */
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.Height = height;
			swapChainDesc.Width = width;
			/** 不进行多重采样 */
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.Scaling = DXGI_SCALING_NONE; /**< 当窗口被拉伸时不进行适应性操作 */
			swapChainDesc.Stereo = false;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			XifFailed(m_factory->CreateSwapChainForHwnd(cmdQueue.Get(),
				win->hWnd(), &swapChainDesc,
				nullptr,  /**< 不使用全屏，可直接传入null*/
				nullptr, /**< 暂时不对输出做任何处理*/
				swapChain.GetAddressOf()), {
					return nullptr;
				});
		}

		m_devices.push_back(new DX12RenderDevice(cmdQueue, swapChain, device, width, height));
		return m_devices.back();
	}
}