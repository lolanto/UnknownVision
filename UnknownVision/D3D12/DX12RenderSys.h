#ifndef D3D12_RENDER_SYS_H
#define D3D12_RENDER_SYS_H

#include "DX12_UVConfig.h"
#include "../RenderSys/RenderSys.h"

namespace UnknownVision {
	/** @remark 暂不考虑multi-adapter */
	class DX12_RenderSys : public RenderSys {
	public:
		DX12_RenderSys(API_TYPE type, float width, float height) : 
			RenderSys(type, width, height) {}
		/** 初始化DX12的基础设备，同时也会创建渲染窗口
		 * @param win 显示渲染结果的系统窗口
		 * @return 初始化成功返回true，失败返回false */
		virtual bool Init(WindowBase* win);
	private:
		SmartPTR<IDXGIFactory6> m_factory; /** DXGI基本工厂，初始化所依赖 */
		SmartPTR<ID3D12Device5> m_device; /**< 当前选择的Adapter的代表 */
		SmartPTR<IDXGISwapChain1> m_swapChain; /**< Adapter相关的交换链 */
		SmartPTR<ID3D12CommandQueue> m_mainCmdQueue; /**< 关键的指令提交通道 */
	private:
		SmartPTR<ID3D12DescriptorHeap> m_backBufferRTVDescripotrHeap; /**< 存储后台缓冲绑定的Render Target View Descriptor的Descriptor heap */
		SmartPTR<ID3D12Resource1> m_RenderTargets[BACK_BUFFER_COUNT]; /**< 后台缓冲绑定的Render Targets的缓冲 */
	};
}

#endif // D3D12_RENDER_SYS_H
