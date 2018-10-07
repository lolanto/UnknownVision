#pragma once

class ResMgr_Imp;

class IResMgr {
public:
	static IResMgr& GetInstance();
public:
	// Resource Manager Interface
private:
	IResMgr();
	ResMgr_Imp * _ptr;
};