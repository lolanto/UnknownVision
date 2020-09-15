#pragma once
#include "Component.h"
#include <unordered_map>

class PipelineEntity {
public:
	template<typename T>
	T* GetComponent() {
		auto iter = m_componentIDToComponentPtr.find(T::ID);
		if (iter == m_componentIDToComponentPtr.end()) return nullptr;
		return dynamic_cast<T*>(iter->second);
	}
	template<typename T>
	T* SetComponent(T* newC) {
		T* prev = dynamic_cast<T*>(m_componentIDToComponentPtr[T::ID]);
		m_componentIDToComponentPtr[T::ID] = newC;
		return prev;
	}
	template<typename T>
	bool HasComponent() const { return m_componentIDToComponentPtr.count(T::ID) != 0; }
public:
	PipelineEntity() = default;
	~PipelineEntity() {
		RemoveAllComponents();
	}
	void RemoveAllComponents() {
		for (auto e : m_componentIDToComponentPtr) {
			delete e.second;
		}
		m_componentIDToComponentPtr.clear();
	}
private:
	std::unordered_map<size_t, IComponent*> m_componentIDToComponentPtr;
};
