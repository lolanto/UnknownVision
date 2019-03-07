#ifndef UV_BUFFER_H
#define UV_BUFFER_H

#include "Resource.h"

namespace UnknownVision {
	class Buffer : public Resource {
	public:
		Buffer(size_t byteSize, size_t numEle, BufferType type, BufferFlagCombination flag, uint32_t RID)
			: m_byteSize(byteSize), m_numEle(numEle), m_type(type), m_flag(flag), Resource(RID) {}
		~Buffer() = default;
		size_t ByteSize() const { return m_byteSize; }
		size_t NumberOfElements() const { return m_numEle; }
		size_t ElementSize() const { return m_byteSize / m_numEle; }
		BufferType Type() const { return m_type; }
		BufferFlagCombination Flag() const { return m_flag; }
	protected:
		const size_t m_byteSize; // 整个缓冲的大小
		const size_t m_numEle; // 缓冲中的元素数量
		const BufferFlagCombination m_flag = BufferFlag::BF_INVALID;
		const BufferType m_type;
	};
}

#endif // UV_BUFFER_H
