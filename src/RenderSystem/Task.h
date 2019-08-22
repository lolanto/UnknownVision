#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
#include "RenderDescriptor.h"
#include <atomic>
#include <vector>

BEG_NAME_SPACE

struct Command {
	enum Type : uint8_t {
		COMMAND_TYPE_TEST = 0U,
		COMMAND_TYPE_UPDATE_BUFFER_FROM_SYSTEM_MEMORY = 0x01U,
		COMMAND_TYPE_EXECUTE_PROGRAM = 0x02U
	};

	Command(Command&& cmd) : type(cmd.type) {
		parameters.swap(cmd.parameters);
		extraData.swap(cmd.extraData);
	}
	Command(Type type) : type(type) {}
	Command(const Command&) = delete;
	Command& operator=(const Command&) = delete;
	Command& operator=(Command&&) = delete;

	std::vector<Parameter> parameters;
	std::vector<std::byte> extraData;
	const Type type;
};

/** 指令的载体，一个任务由其所包含的指令序列定义，一个任务只能有一份 */
struct Task {

	Task() = default;
	Task& operator=(const Task& task) = delete;
	Task(const Task& task) = delete;
	Task& operator=(Task&& task) {
		Frame = task.Frame;
		Commands.swap(task.Commands);
		task.Reset();
		return *this;
	};
	Task(Task&& task) : Frame(task.Frame), Commands(std::move(task.Commands)) { task.Reset(); }

	/** 清空所有的指令和其它状态 */
	void Reset() { Frame = UINT64_MAX; Commands.clear(); }

	void Test(BufferDescriptor buf);

	void UpdateBufferFromMemory(BufferDescriptor buf, void* data, size_t size);
	
	void ExecuteProgram(ProgramHandle pmg, std::vector<Parameter> parameters) {}

	size_t NumberOfCommands() const { return Commands.size(); }

	std::vector<Command> Commands;
	TaskFrame Frame = TaskFrame::InvalidIndex(); /**< 任务提交时的逻辑帧，不要手动修改 */
};

END_NAME_SPACE
