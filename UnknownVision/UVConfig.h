#pragma once
#include <map>

typedef std::map<std::string, void*> NameParams;
typedef std::pair<std::string, void*> NameParamPair;
typedef uint32_t UINT;

enum API_TYPE {
	DirectX11_0 = 0
};

