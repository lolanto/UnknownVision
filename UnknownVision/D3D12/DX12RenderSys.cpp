#include "DX12RenderSys.h"
#include "../Utility/WindowBase/win32/WindowWin32.h"

namespace UnknownVision {
	bool DX12_RenderSys::Init(WindowBase* win) {
		HRESULT hr = S_OK;
		/** 创建DXGI工厂!  该过程可用于激活Debug Layer */
		UINT factoryFlag = 0;
#ifdef DEBUG
		factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
#endif // DEBUG
		if (FAILED(CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&m_factory)))) {
			MLOG(LE, "create dxgi factory failed!");
			return false;
		}
		/** 枚举设备 */
		{
			SmartPTR<IDXGIAdapter4> adapter;
			bool succeed = false;
			for (UINT adapterIdx = 0; m_factory->EnumAdapters(adapterIdx,
				reinterpret_cast<IDXGIAdapter**>(adapter.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND; ++adapterIdx) {
				DXGI_ADAPTER_DESC3 adapterDesc;
				adapter->GetDesc3(&adapterDesc);
				if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; /**< 不使用软件模拟的设备 */
				/** 检查该设备是否支持dx12 */
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device5), nullptr))) {
					succeed = true;
					break;
				}
			}
			if (!succeed) {
				MLOG(LE, "there's no appropriate adapter can be used!");
				return false;
			}
			if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)))) {
				MLOG(LE, "create d3d12 device failed!");
				return false;
			}
		}
		/** 创建关键指令提交队列 */
		{
			D3D12_COMMAND_QUEUE_DESC queDesc;
			queDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queDesc.NodeMask = 0; /**< 不使用多设备 */
			queDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH; /**< CommandQueue需要分配优先级，这里测试使用高优先级 */
			if (FAILED(m_device->CreateCommandQueue(&queDesc, IID_PPV_ARGS(&m_mainCmdQueue)))) {
				MLOG(LE, "create main command queue failed!");
				return false;
			}
		}
		/** 创建交换链 */
		{
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; /**< 指定交换链缓冲的透明行为，暂且不进行特殊设置 */
			swapChainDesc.BufferCount = BACK_BUFFER_COUNT; /**< 指定后台缓冲的数量 */
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; /**< 交换链缓冲的用途，默认已有backbuffer */
			swapChainDesc.Flags = 0; /**< 不进行特殊设置 */
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.Height = static_cast<UINT>(m_basicHeight);
			swapChainDesc.Width = static_cast<UINT>(m_basicWidth);
			/** 不进行多重采样 */
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.Scaling = DXGI_SCALING_NONE; /**< 当窗口被拉伸时不进行适应性操作 */
			swapChainDesc.Stereo = false;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			if (FAILED(m_factory->CreateSwapChainForHwnd(m_mainCmdQueue.Get(),
				reinterpret_cast<WindowWin32*>(win)->hWnd(), &swapChainDesc,
				nullptr /**< 不使用全屏，可直接传入null*/,
				nullptr /**< 暂时不对输出做任何处理*/,
				m_swapChain.ReleaseAndGetAddressOf()))) {
				MLOG(LE, "create swap chain failed!");
				return false;
			}
		}
		/** 创建RTV Descriptor heap 并存储Descriptor */
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
			heapDesc.NodeMask = 0;
			heapDesc.NumDescriptors = BACK_BUFFER_COUNT;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; /**< RTV类型heap只能选这个 */
			if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backBufferRTVDescripotrHeap)))) {
				MLOG(LE, "create renter target view descriptor heap failed!");
				return false;
			}

			D3D12_CPU_DESCRIPTOR_HANDLE handle = m_backBufferRTVDescripotrHeap->GetCPUDescriptorHandleForHeapStart();
			UINT increrment = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			for (UINT i = 0; i < BACK_BUFFER_COUNT; ++i) {
				m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i]));
				m_device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, handle);
				handle.ptr += increrment;
			}
		}
		return true;
	}
}
