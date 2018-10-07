#pragma once
#include "IAlloc.h"
#include <cstdint>
#include <map>

class FreeList : public IAlloc {
public:
	struct budget {
		budget* next;
		budget* last;
		unsigned int capacity;
		unsigned int remain;
	};
public:
	FreeList();
	~FreeList();
	void* Alloc(size_t size);
	void Dealloc(void* ptr);
private:
	void* createBigMem(size_t size);
private:
	/*std::map<uintptr_t, >*/
	budget * head;
	budget* end;
	budget* cur;
};