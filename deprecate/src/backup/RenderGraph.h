#include <cstdint>
#include <vector>
#include <deque>
#include <tuple>
#include <functional>

#define CUSTOM_FLAG_OPERATORS(FLAG, TYPE) \
inline constexpr FLAG operator | (FLAG lhs, FLAG rhs) { return FLAG(static_cast<TYPE>(lhs) | static_cast<TYPE>(rhs)); } \
inline constexpr FLAG operator & (FLAG lhs, FLAG rhs) { return FLAG(static_cast<TYPE>(lhs) & static_cast<TYPE>(rhs)); } \
inline constexpr FLAG operator ^ (FLAG lhs, FLAG rhs) { return FLAG(static_cast<TYPE>(lhs) ^ static_cast<TYPE>(rhs)); } \
inline constexpr FLAG operator ~ (FLAG lhs) { return FLAG(~static_cast<TYPE>(lhs)); }

enum ResourceAccessBit : uint32_t{
	RESOURCE_ACCESS_BIT_NULL = 0x0u,
	RESOURCE_ACCESS_BIT_VERTEX_AND_CONSTANT_BUFFER = 0x00000001u,
	RESOURCE_ACCESS_BIT_INDEX_BUFFER = 0x00000002u,
	RESOURCE_ACCESS_BIT_RENDER_TARGET = 0x00000004u,
	RESOURCE_ACCESS_BIT_PRESENT = 0x00000008u,
	RESOURCE_ACCESS_BIT_DEPTH_WRITE = 0x00000010u,
	RESOURCE_ACCESS_BIT_DEPTH_READ = 0x00000020u,
	RESOURCE_ACCESS_BIT_NON_PIXEL_SHADER_RESOURCE = 0x00000040u,
	RESOURCE_ACCESS_BIT_PIXEL_SHADER_RESOURCE = 0x00000080u
};

CUSTOM_FLAG_OPERATORS(ResourceAccessBit, uint32_t);

enum TaskType : uint8_t {
	TASK_TYPE_INVALID = 0x0u,
	TASK_TYPE_PASS = 0x01u,
	/** BARRIER */
	TASK_TYPE_BARRIER = 0x02u,
	TASK_TYPE_BARRIER_BEG = 0x04u,
	TASK_TYPE_BARRIER_END = 0x08u,
};

struct PassResourceAccessInfo {
	struct PassResourceAccess {
		uint32_t resourceIndex;
		ResourceAccessBit accessBits;
	};
	std::vector<PassResourceAccess> resourceAccesses;
};

struct TaskUnit {
	TaskType type;
	struct ResourceBarrier {
		uint32_t resourceIndex;
		ResourceAccessBit before;
		ResourceAccessBit after;
		ResourceBarrier() = default;
		ResourceBarrier(uint32_t rid, ResourceAccessBit bef, ResourceAccessBit aft)
			: resourceIndex(rid), before(bef), after(aft) {}
		bool operator==(const ResourceBarrier& rhs) const {
			return !memcmp(this, &rhs, sizeof(ResourceBarrier));
		}
	};
	union {
		uint32_t passIndex;
		ResourceBarrier barrier;
	};
	TaskUnit() : type(TASK_TYPE_INVALID) {}
	TaskUnit(uint32_t passIndex) : type(TASK_TYPE_PASS), passIndex(passIndex) {}
	TaskUnit(uint32_t resourceIndex, ResourceAccessBit before, ResourceAccessBit after, TaskType type)
		: type(type), barrier({ resourceIndex, before, after}) {}
	bool IsPass() const { return type == TASK_TYPE_PASS; }
	bool IsBarrier() const { return static_cast<uint8_t>(type) >= static_cast<uint8_t>(TASK_TYPE_BARRIER) &&
			static_cast<uint8_t>(type) <= static_cast<uint8_t>(TASK_TYPE_BARRIER_END); }
	bool operator==(const TaskUnit& rhs) const {
		if (rhs.type != type) return false;
		switch (type)
		{
		case TASK_TYPE_PASS:
			return passIndex == rhs.passIndex;
		case TASK_TYPE_BARRIER:
		case TASK_TYPE_BARRIER_BEG:
		case TASK_TYPE_BARRIER_END:
			return barrier == rhs.barrier;
		default:
			return false;
		}
	}
	/** 两个任务是否能够交换 */
	bool CanSwitch(const TaskUnit& rhs, const std::vector<PassResourceAccessInfo>& passes) const {
		if (IsPass()) {
			if (rhs.IsPass()) return true;
			if (rhs.IsBarrier()) {
				/** pass 不依赖这个barrier */
				for (const auto& resource : passes[passIndex].resourceAccesses) {
					if (resource.resourceIndex == rhs.barrier.resourceIndex) return false;
				}
				return true;
			}
			return false;
		}
		if (IsBarrier()) {
			if (rhs.IsBarrier()) return true;
			if (rhs.IsPass()) {
				/** pass 不依赖 barrier */
				for (const auto& resource : passes[rhs.passIndex].resourceAccesses) {
					if (resource.resourceIndex == barrier.resourceIndex) return false;
				}
				return true;
			}
			return false;
		}
		return false;
	}
};

std::ostream& operator<< (std::ostream& lhs, const TaskUnit& rhs);

class RenderGraph {
private:
	struct ResourceStateElement {
		ResourceAccessBit accessBits;
		int32_t lastTaskIndex;
		ResourceStateElement() : accessBits(RESOURCE_ACCESS_BIT_NULL), lastTaskIndex(-1) {}
	};
	/** 辅助函数，将tasks某个元素放入到指定的另外一个位置
	 * @param tasks 需要处理的任务列表
	 * @param from task队列中被修改的task的索引
	 * @param to 将task插入到 to, to - 1 之间*/
	inline void moveTask(std::vector<TaskUnit>& tasks, int from, int to) {
		if (from == to) return;
		TaskUnit temp = tasks[from];
		/** to在前 */
		if (from > to) {
			memmove(tasks.data() + to + 1, tasks.data() + to, (from - to) * sizeof(TaskUnit));
			tasks[to] = temp;
		}
		/** from在前 */
		else {
			memmove(tasks.data() + from, tasks.data() + from + 1, (to - from - 1) * sizeof(TaskUnit));
			tasks[to - 1] = temp;
		}
	}
private:
	inline void analyse_extendPhase(const std::vector<PassResourceAccessInfo>& passes, uint32_t numRes,
		const std::function<void(int, int)>& maintain_toBeOptimizeBarriers_Consistency,
		std::vector<TaskUnit>& tasks, std::deque<uint32_t>& toBeOptimizedBarriers);

	inline void analyse_reviewPhase(const std::vector<PassResourceAccessInfo>& passes,
		const std::function<void(int, int)>& maintain_toBeOptimizeBarriers_Consistency,
		std::vector<TaskUnit>& tasks, std::deque<uint32_t>& toBeOptimizedBarriers);
public:
	std::vector<TaskUnit> Analyse(const std::vector<PassResourceAccessInfo>& passes, uint32_t numRes);
};
