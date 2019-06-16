#include "DX12RenderBasic.h"

#define XifFailed(function, behavior) if (FAILED(function)) behavior

BEG_NAME_SPACE

constexpr uint64_t UploadBufferSize = 1024;
bool DX12RenderDevice::Initialize(std::string config)
{
	/** 初始化必须的组件 */

	return true;
}

BufferHandle DX12RenderDevice::RequestBuffer(size_t size, ResourceUsage usage, ResourceFlag flag, bool transient) thread_safe {
	return BufferHandle::InvalidIndex();
}

END_NAME_SPACE
