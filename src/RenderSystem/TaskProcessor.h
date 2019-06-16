#pragma once
#include "RenderSystemConfig.h"
#include "Task.h"
#include <functional>
BEG_NAME_SPACE
class TaskProcessor {
public:
	virtual ~TaskProcessor() = default;
	virtual std::function<bool()> Process(Task&) = 0;
};
END_NAME_SPACE
