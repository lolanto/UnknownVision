#include "UVFactory.h"
#include <iostream>
#include <cassert>
using UnknownVision::UVFactory;
using UnknownVision::RenderSys;
using UnknownVision::BufferMgr;
using UnknownVision::ShaderMgr;
using UnknownVision::Texture2DMgr;

int main() {

	// 测试使用的顶点数据
	float vtxData[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	};

	UVFactory& factory = UVFactory::GetInstance();
	if (!factory.Init(DirectX11_0, 720, 640)) return 0;
	RenderSys& rs = factory.GetRenderSys();
	ShaderMgr& sm = rs.ShaderManager();
	BufferMgr& bm = rs.BufferManager();
	Texture2DMgr& t2dm = rs.Texture2DManager();

	int vs = sm.CreateShaderFromBinaryFile(UnknownVision::ST_Vertex_Shader, u8"../Debug/VertexShader.cso");
	int ps = sm.CreateShaderFromBinaryFile(UnknownVision::ST_Pixel_Shader, u8"../Debug/PixelShader.cso");
	assert(vs != -1);
	std::vector<UnknownVision::SubVertexAttributeLayoutDesc> inputLayout;
	inputLayout.assign({
		{u8"POSITION", 0, UnknownVision::VADT_FLOAT3, 0, 0},
	});
	int il = rs.CreateInputLayout(inputLayout, vs);
	assert(il != -1);
	int vb = bm.CreateVertexBuffer(3, 12, reinterpret_cast<uint8_t*>(vtxData));
	assert(vb != -1);
	int vp = rs.CreateViewPort({ 0, 0, rs.Width(), rs.Height(), 0.0f, 1.0f });
	rs.ActiveViewPort(vp);
	// 设置渲染管线状态
	rs.BindVertexBuffer(vb);
	rs.ActiveInputLayout(il);
	rs.BindShader(vs);
	rs.BindShader(ps);
	rs.SetPrimitiveType(UnknownVision::PRI_Triangle);
	rs.BindRenderTarget(-1);
	
	rs.Run([&rs]() {
		rs.ClearRenderTarget(-1);
		rs.Draw();
		rs.Present();
	});
	return 0;
}
