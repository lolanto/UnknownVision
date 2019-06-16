#include "ResourceFactory.h"

namespace UnknownVision {
	Token ResourceFactory::ProcessOrder(const ResourceOrder& order) {
		switch (order.type)
		{
		case RESOURCE_TYPE_BUFFER:
			if (!m_bufferFtr) return INVALID_TOKEN;
			return m_bufferFtr->ProcessOrder(order);
			break;
		default:
			return INVALID_TOKEN;
		}
	}
}
