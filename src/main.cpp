//#include "D3D12/DX12RenderSys.h"
//#include "Utility/WindowBase/win32/WindowWin32.h"
#include <iostream>
#include <cassert>
#include <d3d12shader.h>
#include <d3dcompiler.h>

#include "Utility/DXILCompilerHelper/DXCompilerHelper.h"
#include "Utility/FileContainer/FileContainer.h"
#include "RenderSys/ShaderDescription.h"

using UnknownVision::ShaderDescription;

int main() {

	// 测试使用的顶点数据
	float vtxData[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	};

	//WindowWin32 win("default", 300, 300, true);
	//win.Init();
	//UnknownVision::DX12_RenderSys rd(DirectX12_0, 300, 300);
	//rd.Init(&win);
	DXCompilerHelper helper;
	std::vector<uint8_t> byteCodes;
	std::vector<char> err;
	if (!helper.CompileToByteCode("PixelShader.hlsl", "ps_6_0", byteCodes, false,&err)) {
		std::cout << err.data();
	}
	FileContainer fc("VertexShader.cso", std::ios::out | std::ios::trunc);
	fc.WriteFile(0, byteCodes.size(), reinterpret_cast<char*>(byteCodes.data()));
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shrReflect;
	ShaderDescription desc;
	if (helper.RetrieveShaderDescriptionFromByteCode(byteCodes, desc)) {
		UnknownVision::PrintShaderDescriptionToConsole(desc);
	}
	system("pause");
	return 0;
}
