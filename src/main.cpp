#include "D3D12/DX12RenderBasic.h"
#include "Utility/WindowBase/win32/WindowWin32.h"
#include <iostream>
#include <random>
#include <stack>

using namespace UnknownVision;

int main() {
	WindowWin32 win("test", 100, 100, true);
	win.Init();
	DX12RenderBackend rb;
	rb.Initialize();
	DX12BackendUsedData bud = { &win, win.Width(), win.Height() };
	auto rd = reinterpret_cast<DX12RenderDevice*>(rb.CreateDevice(&bud));
	
	ShaderNames names("vs", "ps");
	VertexAttributeDescs descs = {
		SubVertexAttributeDesc(VERTEX_ATTRIBUTE_TYPE_POSITION, ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
		0, 0, 0),
		SubVertexAttributeDesc(VERTEX_ATTRIBUTE_TYPE_NORMAL, ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
		0, 0, 0),
		SubVertexAttributeDesc(VERTEX_ATTRIBUTE_TYPE_TEXTURE, ELEMENT_FORMAT_TYPE_R32G32_FLOAT,
		0, 0, 0),
		SubVertexAttributeDesc(VERTEX_ATTRIBUTE_TYPE_TANGENT, ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
		0, 0, 0)
	};
	auto descHandle = rb.RegisterVertexAttributeDescs(descs);
	assert(descHandle != descHandle.InvalidIndex());
	RasterizeOptions rastOpt;
	OutputStageOptions osOpt;
	osOpt.rtvFormats[0] = ELEMENT_FORMAT_TYPE_R16_FLOAT;
	auto programDescirptor = rb.RequestProgram(names, descHandle, true, rastOpt, osOpt);

	std::cout << rd->generateGraphicsPSO(programDescirptor) << '\n';

	std::cout << "Finished!";
	std::cin.get();
	return 0;
}
