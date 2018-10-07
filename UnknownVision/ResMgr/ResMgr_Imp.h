#pragma once
#include "IAlloc.h"
class ResMgr_Imp {
public:
	static ResMgr_Imp* GetInstance();
	void* Alloc(size_t size);
	void Dealloc(void* ptr);
private:
	ResMgr_Imp() {}
	IAlloc * m_alloc;
};