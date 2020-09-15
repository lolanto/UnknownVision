#pragma once
#include "GPUResource.h"
#include <vector>
#include <memory>
BEG_NAME_SPACE

/** 静态不可变更的顶点缓冲 */
class StaticVertexBuffer : public GPUResource {
public:
	static std::unique_ptr<StaticVertexBuffer> Create(size_t count = 0, size_t elementSize = 0, void* pData = nullptr, size_t node = 0);
public:
	StaticVertexBuffer() : m_count(0), m_elementSize(0), m_pData(nullptr) {
		m_status.usage = RESOURCE_USAGE_VERTEX_BUFFER;
		m_status.flag = RESOURCE_FLAG_STABLY;
	}
	virtual ~StaticVertexBuffer() {}
	void SetInitialData(size_t count, size_t elementSize, void* pData) {
		m_count = count;
		m_elementSize = elementSize;
		m_pData = pData;
	}
	virtual void Release() {
		m_count = 0;
		m_pData = nullptr;
	}
protected:
	size_t m_count;
	size_t m_elementSize;
	void* m_pData;
};


class StaticIndexBuffer : public GPUResource {
public:
	static std::unique_ptr<StaticIndexBuffer> Create(size_t count = 0, void* pData = nullptr, size_t node = 0);
private:
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
	void* m_pData; // 不保证长时间有效
};

END_NAME_SPACE

