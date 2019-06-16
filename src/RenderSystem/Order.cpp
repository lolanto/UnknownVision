#include "Order.h"
#include <string>

namespace UnknownVision {
	OrderHash ResourceOrder::Hash() const {
		std::string h(sizeof(ResourceOrder) + initData.size(), 0);
		/** 向字符串的内存空间填充数据 */
		memcpy(h.data(), this, sizeof(ResourceOrder));
		memcpy(h.data() + sizeof(ResourceOrder), initData.data(), initData.size());
		std::hash<std::string> strHash;
		return strHash(h);
	}
	PassHash PassOrder::Hash() const
	{
		std::string h(sizeof(PassOrder) + (create.size() +
			write.size() + read.size()) * sizeof(ResourceUsageBriefInfo), 0);
		/** 向字符串的内存空间填充数据 */
		char* hD = h.data();
		memcpy(hD, this, sizeof(PassOrder));
		hD += sizeof(PassOrder);
		memcpy(hD, create.data(), create.size() * sizeof(ResourceUsageBriefInfo));
		hD += create.size() * sizeof(ResourceUsageBriefInfo);
		memcpy(hD, write.data(), write.size() * sizeof(ResourceUsageBriefInfo));
		hD += write.size() * sizeof(ResourceUsageBriefInfo);
		memcpy(hD, read.data(), read.size() * sizeof(ResourceUsageBriefInfo));
		std::hash<std::string> strHash;
		return strHash(h);
	}
}
