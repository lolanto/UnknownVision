#include "UVFactory.h"
#include <iostream>
using UnknownVision::UVFactory;
using UnknownVision::RenderSys;
using UnknownVision::BufferMgr;
using UnknownVision::ShaderMgr;
using UnknownVision::Texture2DMgr;

int main() {
	UVFactory& factory = UVFactory::GetInstance();
	if (!factory.Init(DirectX11_0, 100, 100)) return 0;
	RenderSys& rs = factory.GetRenderSys();
	ShaderMgr& sm = rs.ShaderManager();
	BufferMgr& bm = rs.BufferManager();
	Texture2DMgr& t2dm = rs.Texture2DManager();

	int vs = sm.CreateShaderFromBinaryFile(UnknownVision::ST_Vertex_Shader, u8"../Debug/InputLayout.cso");
	std::vector<UnknownVision::SubVertexAttributeLayoutDesc> inputLayout;
	inputLayout.assign({
		{u8"POSITION", 0, UnknownVision::VADT_FLOAT4, 0, 0}
	});
	int il = rs.CreateInputLayout(inputLayout, vs);
	std::cout << il << std::endl;
	system("pause");
	return 0;
}
