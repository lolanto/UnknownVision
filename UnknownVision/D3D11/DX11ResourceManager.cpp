#include "DX11ResourceManager.h"
#include "../Utility/InfoLog/InfoLog.h"

void DX11_CanvasManager::GetResourceData(Resource* re, NameParams& param) {
	// initialize return value
	NameParamPair np_srv = std::make_pair<std::string, void*>("SRV", nullptr);
	NameParamPair np_rtv = std::make_pair<std::string, void*>("RTV", nullptr);
	NameParamPair np_tex2d = std::make_pair<std::string, void*>("TEX2D", nullptr);
	param.insert(np_srv);
	param.insert(np_rtv);
	param.insert(np_tex2d);
	// Get resource id and search datas
	Canvas* ptr = static_cast<Canvas*>(re);
	DX11_UV::CanvasDataContainer::iterator iter = m_datas.find(ptr->RID);
	if (iter == m_datas.end()) {
		MLOG(LW, __FUNCTION__, LL, "can not find canvas resource!");
		return;
	}
	// setup return values
	param.at("SRV") = iter->second.srv.Get();
	param.at("RTV") = iter->second.rtv.Get();
	param.at("TEX2D") = iter->second.tex2d.Get();
	return;
}

Canvas DX11_CanvasManager::Create(std::string name, UINT usage, char* path) {

}