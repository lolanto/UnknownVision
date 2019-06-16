#include "RenderGraph.h"

#include<iostream>
std::ostream& operator<< (std::ostream& lhs, const TaskUnit& rhs) {
	switch (rhs.type)
	{
	case TASK_TYPE_PASS:
		lhs << 'P' << rhs.passIndex << ' ';
		break;
	case TASK_TYPE_BARRIER_BEG:
		lhs << "T^ " << rhs.barrier.resourceIndex << ' ';
		break;
	case TASK_TYPE_BARRIER_END:
		lhs << "T_ " << rhs.barrier.resourceIndex << ' ';
		break;
	case TASK_TYPE_BARRIER:
		lhs << "T " << rhs.barrier.resourceIndex << ' ';
		break;
	default:
		break;
	}
	return lhs;
}

inline void RenderGraph::analyse_extendPhase(const std::vector<PassResourceAccessInfo>& passes, uint32_t numRes, 
	const std::function<void(int, int)>& maintain_toBeOptimizeBarriers_Consistency, 
	std::vector<TaskUnit>& tasks, std::deque<uint32_t>& toBeOptimizedBarriers) {
	std::vector<ResourceStateElement> resStates(numRes, ResourceStateElement());
	/** 在任务队列被move后，需要调用该方法维护resStates变量的一致性
	* @param from task队列中被修改的task的索引
	* @param to 将task插入到 to, to - 1 之间 */
	auto maintain_resStates_Consistency = [&resStates](int from, int to) {
		if (from == to) return;
		if (to > from) { /** from在前 */
			for (auto& state : resStates) {
				if (state.lastTaskIndex == from) state.lastTaskIndex = to - 1;
				else if (state.lastTaskIndex > from && state.lastTaskIndex < to) --state.lastTaskIndex;
			}
		}
		else { /** to在前 */
			for (auto& state : resStates) {
				if (state.lastTaskIndex == from) state.lastTaskIndex = to;
				else if (state.lastTaskIndex >= to && state.lastTaskIndex < from) ++state.lastTaskIndex;
			}
		}
	};
	tasks.reserve(passes.size() + 2 * numRes);
	for (int passIndex = 0; passIndex < passes.size(); ++passIndex) {
		const auto& pass = passes[passIndex];
		bool canMoveForward = true;
		/** 遍历该pass所依赖的所有resources */
		for (const auto& resource : pass.resourceAccesses) {
			if (resStates[resource.resourceIndex].accessBits != RESOURCE_ACCESS_BIT_NULL && resStates[resource.resourceIndex].accessBits != resource.accessBits) {
				canMoveForward = false; /** 该pass的执行需要资源状态切换，不能提前 */
										/** 需要资源状态切换 */
				tasks.push_back(TaskUnit(resource.resourceIndex, resStates[resource.resourceIndex].accessBits, resource.accessBits, TASK_TYPE_BARRIER_BEG));
				bool optimized = false;
				int tpos = tasks.size() - 2;
				for (; tpos != resStates[resource.resourceIndex].lastTaskIndex; --tpos) {
					if (tasks[tpos].IsBarrier()) continue; /** barrier之间可以交换顺序 */
					if (tasks[tpos].IsPass()) {
						if (tasks[tpos].CanSwitch(tasks.back(), passes)) {
							/** 当前可以穿过该pass，则该barrier可以被优化 */
							optimized = true;
						}
						else break;
					}
				}
				/** 假如该barrier可以移动，tpos + 1指向可以越过的，最左边的task的位置 */
				++tpos;
				if (optimized) {
					moveTask(tasks, tasks.size() - 1, tpos);
					maintain_resStates_Consistency(tasks.size() - 1, tpos);
					maintain_toBeOptimizeBarriers_Consistency(tasks.size() - 1, tpos);
				}
				else {
					/** 该barrier没有被优化，将barrier_end的下标压入待优化barrier列表中 */
					toBeOptimizedBarriers.push_back(tasks.size());
				}
				tasks.push_back(TaskUnit(resource.resourceIndex, resStates[resource.resourceIndex].accessBits, resource.accessBits, TASK_TYPE_BARRIER_END));
			}
		}
		/** 插入该pass */
		tasks.push_back(TaskUnit(passIndex));
		int passTaskFinalPos = tasks.size() - 1; /**< 这个pass task最后落在tasks中的下标位置 */
		if (canMoveForward) {
			/** 向前移动pass，尽可能到达需要优化的barrier中间 */
			/** pass可以移动的最左边的位置，假如有需要被优化的barrier存在，则应该尽可能往该(split)barrier的中心移动 */
			int leftMost = toBeOptimizedBarriers.empty() ? 0 : toBeOptimizedBarriers.front();
			int ppos = tasks.size() - 2;
			for (; ppos >= leftMost && tasks[ppos].CanSwitch(tasks.back(), passes); --ppos) {}
			/** for退出的条件是pass已经 超过 了最左边或者不能越过当前ppos指向的task，
			* 所以当前可以越过的task的下标应该是 ppos + 1 */
			++ppos;
			if (ppos != tasks.size() - 1) { /** 当前pass可以移动 */
				for (auto barrier = toBeOptimizedBarriers.begin();
					barrier != toBeOptimizedBarriers.end(); ++barrier) {
					/** 尽可能优化靠左边的barrier */
					if (ppos >= *barrier) {
						ppos = *barrier;
						toBeOptimizedBarriers.erase(barrier);
						break;
					}
				}
				moveTask(tasks, tasks.size() - 1, ppos);
				maintain_resStates_Consistency(tasks.size() - 1, ppos);
				maintain_toBeOptimizeBarriers_Consistency(tasks.size() - 1, ppos);
				passTaskFinalPos = ppos;
			}
		}
		/** 更新该pass task使用到的所有资源的使用情况
		* 让resStates中的记录务必是使用该资源的最后一个pass的task索引 */
		for (const auto& resource : pass.resourceAccesses) {
			resStates[resource.resourceIndex].accessBits = resource.accessBits;
			resStates[resource.resourceIndex].lastTaskIndex =
				resStates[resource.resourceIndex].lastTaskIndex > passTaskFinalPos ?
				resStates[resource.resourceIndex].lastTaskIndex : passTaskFinalPos;
		}
	}
}

inline void RenderGraph::analyse_reviewPhase(const std::vector<PassResourceAccessInfo>& passes, 
	const std::function<void(int, int)>& maintain_toBeOptimizeBarriers_Consistency, 
	std::vector<TaskUnit>& tasks, std::deque<uint32_t>& toBeOptimizedBarriers) {

	// 伪命题
	// if (toBeOptimizedBarriers.empty()) return;
	///** 让待优化的barrier_end尽可能往后扩张 */
	//for (auto barrier = toBeOptimizedBarriers.begin(); barrier != toBeOptimizedBarriers.end();) {
	//	const TaskUnit& task = tasks[*barrier];
	//	bool optimized = false;
	//	int pos = (*barrier) + 1;
	//	for (; pos < tasks.size() - 1 && tasks[pos].CanSwitch(task, passes); ++pos) {
	//		if (tasks[pos].type == TASK_TYPE_PASS) optimized = true;
	//	}
	//	if (optimized) {
	//		moveTask(tasks, *barrier, pos);
	//		maintain_toBeOptimizeBarriers_Consistency(*barrier, pos);
	//		barrier = toBeOptimizedBarriers.erase(barrier);
	//	}
	//	else ++barrier;
	//}

	if (toBeOptimizedBarriers.empty()) return;
	/** 让待优化的barrier_end之前的pass，尽可能地移动到barrier之间 */
	for (auto barrier = toBeOptimizedBarriers.begin(); barrier != toBeOptimizedBarriers.end();) {
		std::vector<uint32_t> candidatePassTasks; /**< 候选的，可以用于优化当前barrier的pass */
		std::vector<uint32_t> pendingTaskList; /** 当前只扫描到end，没有扫描到beg的barrier的task index */
											   /** tuple的值分别为 1. 当前可用于优化barrier的pass在候选列表中的下标；2. 当前barrier_end前面包含的pass task数量；3. barrier的拷贝 */
		std::vector< std::tuple<int8_t, int8_t, const TaskUnit> > barrierCheckingList; /** 当前pass若想优化barrier必须“跨过”的barrier */
		uint32_t lastBarrierIndex = 0;
		if (barrier != toBeOptimizedBarriers.begin()) {
			lastBarrierIndex = *(barrier - 1);
		}
		uint32_t taskIndex = 0;
		for (taskIndex = *barrier; taskIndex > lastBarrierIndex; --taskIndex) {
			const TaskUnit& task = tasks[taskIndex];
			switch (task.type) {
			case TASK_TYPE_BARRIER_END:
				barrierCheckingList.push_back(std::make_tuple(-1, 0, task));
				pendingTaskList.push_back(task.barrier.resourceIndex);
				break;
			case TASK_TYPE_BARRIER_BEG:
				/** 假如是barrier_beg先出现，表示与之对应的barrier_end在barrier后
				* 此时后续的pass同样要考虑能否跨国该barrier_beg，但由于没有成对出现
				* 所以不将其列入到pending列表中 */
				if ([&barrierCheckingList, &task, &candidatePassTasks]()->bool {
					for (auto bc = barrierCheckingList.rbegin(); bc != barrierCheckingList.rend(); ++bc) {
						if (std::get<2>(*bc).barrier.resourceIndex == task.barrier.resourceIndex) {
							if (std::get<1>(*bc) <= 1 && std::get<0>(*bc) != -1) candidatePassTasks[std::get<0>(*bc)] = UINT32_MAX;
								return false;
						}
					}
					return true;
				}()) {
					barrierCheckingList.push_back(std::make_tuple(-1, 0, task));
				}
				else {
					for (auto pt = pendingTaskList.begin(); pt != pendingTaskList.end(); ++pt) {
						if (*pt == task.barrier.resourceIndex) {
							pendingTaskList.erase(pt);
							break;
						}
					}
				}
				break;
			case TASK_TYPE_PASS:
				if ([&barrierCheckingList, &task, &passes]()->bool {
					for (const auto& bc : barrierCheckingList) {
						if (!std::get<2>(bc).CanSwitch(task, passes))
							return false;
					}
					return true;
				}()) {
					for (auto& bc : barrierCheckingList) {
						std::get<0>(bc) = std::get<0>(bc) == -1 ? pendingTaskList.size() : std::get<0>(bc);
						++std::get<1>(bc);
					}
					pendingTaskList.push_back(taskIndex);
				}
				break;
			default:
				/** 目前而言不处理以上type以外的task */
				break;
			}
		}
		for (; !pendingTaskList.empty(); --taskIndex) {
			const TaskUnit& task = tasks[taskIndex];
			if (task.type == TASK_TYPE_BARRIER_BEG) {
				for (auto bc = barrierCheckingList.rbegin(); bc != barrierCheckingList.rend(); ++bc) {
					if (std::get<2>(*bc).barrier.resourceIndex == task.barrier.resourceIndex) {
						if (std::get<1>(*bc) <= 1 && std::get<0>(*bc) != -1) candidatePassTasks[std::get<0>(*bc)] = UINT32_MAX;
						for (auto pt = pendingTaskList.begin(); pt != pendingTaskList.end(); ++pt) {
							if (*pt == task.barrier.resourceIndex) {
								pendingTaskList.erase(pt);
								break;
							}
						}
						break;
					}
				}
			}
		}
		/** 遍历candidatePassTasks寻找第一个遇到的可用于优化barrier的pass task */
		taskIndex = UINT32_MAX;
		for (const auto passTaskIndex : candidatePassTasks) {
			if (passTaskIndex != UINT32_MAX) {
				taskIndex = passTaskIndex;
				break;
			}
		}
		if (taskIndex == UINT32_MAX) { /** 不存在可优化的pass task */
			++barrier;
		}
		else {
			moveTask(tasks, taskIndex, *barrier);
			maintain_toBeOptimizeBarriers_Consistency(taskIndex, *barrier);
			barrier = toBeOptimizedBarriers.erase(barrier);
		}
	}
	return;
}

std::vector<TaskUnit> RenderGraph::Analyse(const std::vector<PassResourceAccessInfo>& passes, uint32_t numRes) {
	std::vector<ResourceStateElement> resStates(numRes, ResourceStateElement());
	std::vector<TaskUnit> tasks;
	std::deque<uint32_t> toBeOptimizedBarriers;

	/** 在任务队列被move后，需要调用该方法维护toBeOptimizedBarrier变量的一致性
	* @param from task队列中被修改的task的索引
	* @param to 将task插入到 to, to - 1 之间 */
	auto maintain_toBeOptimizeBarriers_Consistency = [&toBeOptimizedBarriers](int from, int to) {
		if (from == to) return;
		if (to > from) { /** from在前 */
			for (auto& barrier : toBeOptimizedBarriers) {
				if (barrier == from) barrier = to - 1;
				if (barrier > from && barrier < to) --barrier;
			}
		}
		else { /** to在前 */
			for (auto& barrier : toBeOptimizedBarriers) {
				if (barrier == from) barrier = to;
				if (barrier >= to && barrier < from) ++barrier;
			}
		}
	};

	analyse_extendPhase(passes, numRes, maintain_toBeOptimizeBarriers_Consistency, tasks, toBeOptimizedBarriers);
	analyse_reviewPhase(passes, maintain_toBeOptimizeBarriers_Consistency, tasks, toBeOptimizedBarriers);
	return tasks;
}
