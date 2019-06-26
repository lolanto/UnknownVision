#include "DX12RenderBasic.h"
#include <cassert>

#define XifFailed(function, behavior) if (FAILED(function)) behavior

BEG_NAME_SPACE

bool DX12RenderDevice::Initialize(std::string config)
{
	/** 构建特殊资源 */
	for (uint8_t idx = 0; idx < NUMBER_OF_BACK_BUFFERS; ++idx) {
		XifFailed(m_swapChain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx])), {
			FLOG("Create Back buffer %d FAILED!\n", idx);
			return false;
		});
	}
	m_textures.insert(std::make_pair(TextureHandle(DEFAULT_BACK_BUFFER), TextureInfo(m_backBuffers[0].Get(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, ScreenWidth, ScreenHeight)));
#ifdef _DEBUG
	/** 检查所有的特殊资源都已经构建完成 */
	for (uint8_t i = 0; i < NUMBER_OF_SPECIAL_BUFFER_RESOURCE; ++i) {
		auto res = m_buffers.find(BufferHandle(i));
		assert(res != m_buffers.end() && res->second.ptr != nullptr);
	}
	for (uint8_t i = 0; i < NUMBER_OF_SPECIAL_TEXTURE_RESOURCE; ++i) {
		auto res = m_textures.find(TextureHandle(i));
		assert(res != m_textures.end() && res->second.ptr != nullptr);
	}
#endif // _DEBUG

	/** 初始化必须的组件 */
	RenderDevice::Initialize(config);
	return true;
}

void DX12RenderDevice::Process()
{
	if (m_state == DEVICE_STATE_RUNNING) {
		Task curTask;
		{
			std::lock_guard<OptimisticLock> lg(m_taskQueueLock);
			if (m_taskQueue.empty()) return;
			curTask = std::move(m_taskQueue.front());
			m_taskQueue.pop();
		}
	/** TODO: 需要逻辑在这里使用接收到的descriptor创建适当的资源 */
	}
}


END_NAME_SPACE
