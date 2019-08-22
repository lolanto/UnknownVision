#include "DX12RenderBasic.h"

BEG_NAME_SPACE

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
		case Command::COMMAND_TYPE_UPDATE_BUFFER_FROM_SYSTEM_MEMORY:
			UPDATE_BUFFER_FROM_SYSTEM_MEMORY_func(cmd);
			break;
		default:
			FLOG("%s: command %d is not found!\n", __FUNCTION__, cmd.type);
		}
	}
}

void DX12RenderDevice::TEST_func(const Command& cmd) {
	FLOG("Command Type is %d\n", cmd.type);
	FLOG("Number of parameters: %zu\nSize of extra data: %zu\n",
		cmd.parameters.size(), cmd.extraData.size());
}

void DX12RenderDevice::UPDATE_BUFFER_FROM_SYSTEM_MEMORY_func(const Command& cmd) {
	auto bufDesc = cmd.parameters[0].buf;
	const void* src = reinterpret_cast<const void*>(cmd.extraData.data());
	const size_t size = *(reinterpret_cast<const size_t*>(cmd.extraData.data() + sizeof(void*)));
	void* dest = nullptr;
	if (bufDesc.handle == InvalidHandleIndex(bufDesc.handle)) {
		/** 临时变量 */
		MLOG("Temperary variable!\n");
	}
	else {
		if (m_buffers.find(bufDesc.handle) == m_buffers.end()) {
			assert(generateBuffer(bufDesc));
		}
		m_buffers[bufDesc.handle].ptr->Map(0, nullptr, &dest);
		memcpy(dest, src, size);
		m_buffers[bufDesc.handle].ptr->Unmap(0, nullptr);
	}
}

END_NAME_SPACE
