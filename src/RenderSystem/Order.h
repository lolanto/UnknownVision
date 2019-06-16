#ifndef UV_ORDER_H
#define UV_ORDER_H

#include <cstdint>
#include <vector>
#include "../UVConfig.h"

namespace UnknownVision {
	using Token = uint32_t; /**< 标签 */
	using OrderHash = uint64_t; /**< 订单的哈希码 */
	using PassHash = uint64_t;
	const Token INVALID_TOKEN = 0xffffffffU;
	struct ResourceOrder {
		OrderHash Hash() const;
		ResourceType type;
		ResourceUsage usage;
		uint32_t width, height;
		uint32_t depth;
		std::vector<uint8_t> initData; /**< 初始化需要的数据 */
	};

	struct ResourceUsageBriefInfo {
		OrderHash hash;
		ResourceType type;
		ResourceUsage usage;
	};

	struct PassOrder {

		PassHash Hash() const;
		/** 一系列的Pass状态 */
		std::vector<ResourceUsageBriefInfo> read;
		std::vector<ResourceUsageBriefInfo> write;
		std::vector<ResourceUsageBriefInfo> create;
		PassType type;
	};

	struct ResourceTransform {
		OrderHash hash;
		ResourceUsage before;
		ResourceUsage after;
	};

	struct Task {
		enum Type {
			TASK_TYPE_RUN_PASS = 0,
			TASK_TYPE_RESOURCE_TRANSFORM,
			TASK_TYPE_SYNC_SIGNAL,
			TASK_TYPE_SYNC_WAIT
		};
		Type type;
		union {
			PassHash pass; /**< 需要执行的pass */
			ResourceTransform transform; /**< 需要进行转换的资源及其前后状态 */
			uint64_t syncSignalID; /**< 触发同步ID */
			uint64_t syncWaitID; /**< 等待同步ID */
		};
	};
}

#endif // UV_ORDER_H
