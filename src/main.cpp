#include "D3D12/DX12RenderBasic.h"
#include "D3D12/DX12ResourceManager.h"
#include "Utility/WindowBase/win32/WindowWin32.h"
#include <iostream>
#include <random>
#include <stack>
#include "RenderSystem/Task.h"

using namespace PROJECT_NAME_SPACE;

ResourceFlag genFlagFromInt(size_t value) {
	switch (value) {
	case 0:
		return RESOURCE_FLAG_INVALID;
	case 1:
		return RESOURCE_FLAG_ONCE;
	case 2:
		return RESOURCE_FLAG_STABLY;
	case 3:
		return RESOURCE_FLAG_FREQUENTLY;
	case 4:
		return RESOURCE_FLAG_READ_BACK;
	default:
		return RESOURCE_FLAG_INVALID;
	}
	return RESOURCE_FLAG_INVALID;
}

std::mutex stMutex;
struct OPRcd {
	ID3D12Resource* ptr;
	size_t size;
};
std::stack<OPRcd> reqStack;
size_t totalReq = 0;
size_t totalRev = 0;

/** 需要将request到的descriptor与实际的资源相互联系 */
/** 需要解决MONZA的错误!! */
int main() {
	WindowWin32 win("test", 100, 100, true);
	win.Init();
	DX12RenderBackend rb;
	rb.Initialize();
	BackendUsedData bud = { &win, 100, 100 };
	DX12RenderDevice* rd = (DX12RenderDevice*)rb.CreateDevice(&bud);
	rd->Initialize("");
	
	std::cout << "Finished!";
	std::cin.get();
	return 0;
}
