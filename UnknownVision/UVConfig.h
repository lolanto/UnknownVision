#pragma once
#include <map>

typedef std::map<std::string, void*> NameParams;
typedef std::pair<std::string, void*> NameParamPair;
typedef unsigned int UINT;

enum API_TYPE {
	DirectX11_0 = 0
};

