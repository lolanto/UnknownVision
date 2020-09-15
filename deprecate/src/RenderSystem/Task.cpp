#include "Task.h"

BEG_NAME_SPACE

void Task::UpdateBufferFromMemory(BufferDescriptor buf, void* data, size_t size) {
	//Command cmd(Command::COMMAND_TYPE_UPDATE_BUFFER_FROM_SYSTEM_MEMORY);
	//cmd.extraData = std::vector<std::byte>(sizeof(data) + sizeof(size));
	//cmd.parameters.push_back({ buf });
	//std::byte* ed = cmd.extraData.data();
	//memcpy(ed, &data, sizeof(data));
	//ed += sizeof(data);
	//memcpy(ed, &size, sizeof(size));
	//Commands.push_back(std::move(cmd));
}

void Task::Test(BufferDescriptor buf) {
	//Command cmd(Command::COMMAND_TYPE_TEST);
	//cmd.parameters.push_back({ buf });

	//Commands.push_back(std::move(cmd));
}

END_NAME_SPACE
