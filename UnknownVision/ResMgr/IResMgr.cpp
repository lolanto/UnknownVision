#include "IResMgr.h"
#include "ResMgr_Imp.h"

IResMgr::IResMgr() {
	_ptr = ResMgr_Imp::GetInstance();
}

IResMgr& IResMgr::GetInstance() {
	static IResMgr _ins;
	return _ins;
}