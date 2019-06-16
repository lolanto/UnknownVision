#include "Task.h"

BEG_NAME_SPACE

void Task::UpdateBuffer(BufferHandle buf, void* data, size_t size) {
	Command newCmd(Command::Type::COMMAND_TYPE_UPDATE_BUFFER);
	newCmd.writeTo.push_back({ buf });
	newCmd.extraData.resize(sizeof(void*) + sizeof(size_t));
	std::byte* ed = newCmd.extraData.data();
	memcpy(ed, &data, sizeof(data)); ed += sizeof(data);
	memcpy(ed, &size, sizeof(size));
	Commands.push_back(std::move(newCmd));
}

void Task::ExecuteProgram(ProgramHandle pmg,
	ProgramParameters input, ProgramParameters output) {

}

END_NAME_SPACE
