#pragma once

class IAlloc {
public:
	virtual void* Alloc(size_t size) = 0;
	virtual void Dealloc(void* ptr) = 0;
	virtual ~IAlloc() {}
};
