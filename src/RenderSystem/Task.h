#pragma once
#include "../UVConfig.h"
#include "RenderSystemConfig.h"
#include <atomic>

BEG_NAME_SPACE
struct Task {
public:
	struct Command {
		enum Type : uint8_t {
			COMMAND_TYPE_UPDATE_BUFFER = 0x01U,
			COMMAND_TYPE_EXECUTE_PROGRAM = 0x02U
		};
		std::vector<Handle> readFrom;
		std::vector<Handle> writeTo;
		std::vector<std::byte> extraData;
		Type type;
		Command(Command&& cmd) {
			readFrom.swap(cmd.readFrom);
			writeTo.swap(cmd.writeTo);
			extraData.swap(cmd.extraData);
			type = cmd.type;
		}
		Command(Type type) : type(type) {}
	};

	Task(uint64_t frame) : Frame(frame) {}
	Task& operator=(const Task& task) = delete;
	Task& operator=(Task&& task) = delete;
	Task(Task&& task) : Frame(task.Frame) { Commands.swap(task.Commands); }
	Task(const Task& task) = delete;

	void UpdateBuffer(BufferHandle buf, void* data, size_t size);
	
	void ExecuteProgram(ProgramHandle pmg, 
		ProgramParameters input, ProgramParameters output);

	size_t NumberOfCommands() const { return Commands.size(); }

	std::vector<Command> Commands;
	const uint64_t Frame;
};

END_NAME_SPACE
