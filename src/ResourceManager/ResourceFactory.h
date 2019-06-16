#ifndef UV_RESOURCE_FACTORY_H
#define UV_RESOURCE_FACTORY_H

#include "../RenderSystem/Order.h"
#include <memory>
namespace UnknownVision {
	class BufferFactory;
	class ResourceFactory {
	public:
		Token ProcessOrder(const ResourceOrder& order);
		void SetBufferFactory(std::unique_ptr<BufferFactory>& bufFtr) { m_bufferFtr.swap(bufFtr); }
	private:
		std::unique_ptr<BufferFactory> m_bufferFtr; /**< 负责创建缓冲的工厂 */
	};

	class BufferFactory {
	public:
		virtual Token ProcessOrder(const ResourceOrder& order) = 0;
	};
}

#endif // UV_RESOURCE_FACTORY_H
