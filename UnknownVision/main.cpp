#include "D3D12/DX12RenderSys.h"
#include "Utility/WindowBase/win32/WindowWin32.h"
#include <iostream>
#include <cassert>

using UnknownVision::DirectX12_0;

int main() {

	// 测试使用的顶点数据
	float vtxData[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	};

	WindowWin32 win("default", 300, 300, true);
	win.Init();
	UnknownVision::DX12_RenderSys rd(DirectX12_0, 300, 300);
	rd.Init(&win);
	system("pause");
	return 0;
}
