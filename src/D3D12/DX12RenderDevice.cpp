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
	if (m_state != DEVICE_STATE_RUNNING) return;

	Task curTask;
	{
		std::lock_guard<OptimisticLock> lg(m_taskQueueLock);
		if (m_taskQueue.empty()) return;
		curTask = std::move(m_taskQueue.front());
		m_taskQueue.pop();
	}
	/** 按顺序处理Task中的指令，同时根据指令类型执行相应的处理函数 */
	for (const auto& cmd : curTask.Commands) {
		switch (cmd.type) {
		case Command::COMMAND_TYPE_TEST:
			TEST_func(cmd);
			break;
		default:
			FLOG("command %d is not found!\n", cmd.type);
		}
	}
}

inline void DX12RenderDevice::fromResourceStatusToHeapTypeAndFlags(const ResourceStatus & status, D3D12_HEAP_TYPE & heapType, D3D12_RESOURCE_FLAGS & flags)
{
	heapType = D3D12_HEAP_TYPE_UPLOAD; /**< 默认heap类型 */
	flags = D3D12_RESOURCE_FLAG_NONE; /**< 默认flag类型 */

	if (status.isStably()) heapType = D3D12_HEAP_TYPE_DEFAULT;
	else if (status.isReadBack()) heapType = D3D12_HEAP_TYPE_READBACK;

	if (status.canBeRenderTarget()) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (status.canBeDepth()) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (status.canBeUnorderAccess()) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	/** TODO: 还有DenyShaderResource, crossAdapter, simultaneousAccess, videoDecodeReferenceOnly */
}

void DX12RenderDevice::TEST_func(const Command & cmd)
{
#ifdef _DEBUG
	assert(cmd.type == Command::COMMAND_TYPE_TEST);
#endif // _DEBUG
	auto& descriptor = cmd.parameters[0].buf;
	if (descriptor.handle == BufferHandle::InvalidIndex()) return;
	if (m_buffers.find(descriptor.handle) != m_buffers.end()) return;
	BufferInfo bufInfo(descriptor.size);
	/** 从resource_flag, resource_usage处理出DX12适用的flag和heapType */
	D3D12_HEAP_TYPE heapType;
	D3D12_RESOURCE_FLAGS flags;
	fromResourceStatusToHeapTypeAndFlags(descriptor.status, heapType, flags);
	{
		auto[resPtr, resState] = m_resourceManager.RequestBuffer(descriptor.size, flags, heapType);
		assert(resPtr != nullptr);
		bufInfo.ptr = resPtr;
		bufInfo.state = resState;
	}
	{
		auto[iter, isInserted] = m_buffers.insert(std::make_pair(descriptor.handle, bufInfo));
		if (isInserted == false) MLOG("Insert Buffer Failed!\n");
	}
	return;
}


END_NAME_SPACE
