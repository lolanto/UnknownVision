#pragma once
#include "GPUResource.h"
#include <vector>
BEG_NAME_SPACE

/** 静态不可变更的顶点缓冲 */
template<typename T>
class StaticVertexBuffer : public GPUResource {
public:
	using VertexDataType = T;
public:
	StaticVertexBuffer() : m_count(0), m_pData(nullptr) {
		m_status.usage = RESOURCE_USAGE_VERTEX_BUFFER;
		m_status.flag = RESOURCE_FLAG_STABLY;
	}
	virtual ~StaticVertexBuffer() {}
	void SetInitialData(size_t count, void* pData) {
		m_count = count;
		m_pData = pData;
	}
	virtual void Release() {
		m_count = 0;
		m_pData = nullptr;
	}
protected:
	size_t m_count;
	void* m_pData;
};


class StaticIndexBuffer : public GPUResource {
public:
	StaticIndexBuffer() : m_count(0), m_pData(nullptr) {
		m_status.usage = RESOURCE_USAGE_INDEX_BUFFER;
		m_status.flag = RESOURCE_FLAG_STABLY;
	}
	virtual ~StaticIndexBuffer() = default;
	void SetInitialData(size_t count, void* pData) {
		m_count = count;
		m_pData = pData;
	}
	virtual void Release() {
		m_count = 0;
		m_pData = nullptr;
	}
protected:
	size_t m_count;
	void* m_pData;
};

END_NAME_SPACE

