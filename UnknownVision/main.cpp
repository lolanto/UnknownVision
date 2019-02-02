#include "UVRoot.h"
#include "RenderSys/RenderSys.h"
#include "ResMgr/IResMgr.h"
#include <iostream>
#include <cassert>
using UnknownVision::Root;
using UnknownVision::RenderSys;
using UnknownVision::BufferMgr;
using UnknownVision::ShaderMgr;
using UnknownVision::Texture2DMgr;
using UnknownVision::VertexDeclarationMgr;
using UnknownVision::ShaderIdx;
using UnknownVision::BufferIdx;
using UnknownVision::VertexDeclarationIdx;
using UnknownVision::RenderTargetIdx;

int main() {

	// 测试使用的顶点数据
	float vtxData[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	};

	Root& factory = Root::GetInstance();
	if (!factory.Init(UnknownVision::DirectX11_0, 720, 640)) return 0;
	RenderSys& rs = factory.GetRenderSys();
	rs.Init();
	ShaderMgr& sm = factory.GetShaderMgr();
	BufferMgr& bm = factory.GetBufferMgr();
	VertexDeclarationMgr& vm = factory.GetVertexDeclarationMgr();

	ShaderIdx vs = sm.CreateShaderFromBinaryFile(UnknownVision::ST_Vertex_Shader, u8"../Debug/VertexShader.cso");
	ShaderIdx ps = sm.CreateShaderFromBinaryFile(UnknownVision::ST_Pixel_Shader, u8"../Debug/PixelShader.cso");
	assert(vs != -1);
	std::vector<UnknownVision::SubVertexAttributeDesc> inputLayout;
	inputLayout.assign({
		{u8"POSITION", 0, UnknownVision::VADT_FLOAT3, 0, 0},
	});
	VertexDeclarationIdx il = vm.CreateVertexDeclaration(inputLayout, vs);
	assert(il != -1);
	BufferIdx vb = bm.CreateVertexBuffer(3, 12, reinterpret_cast<uint8_t*>(vtxData), UnknownVision::BufferFlag::BF_READ_BY_GPU);
	assert(vb != -1);
	UnknownVision::ViewPortDesc vp;
	vp.height = rs.Height(); vp.width = rs.Width(); vp.topLeftX = vp.topLeftY = 0;
	vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
	rs.ActiveViewPort(vp);
	// 设置渲染管线状态
	rs.BindVertexBuffer(vb);
	rs.ActiveVertexDeclaration(il);
	rs.BindShader(vs);
	rs.BindShader(ps);
	rs.SetPrimitiveType(UnknownVision::PRI_TriangleList);
	rs.BindRenderTarget(RenderTargetIdx(-1));

	WindowBase::MainLoop = [&rs](float) {
		rs.ClearRenderTarget(RenderTargetIdx(-1));
		rs.Draw();
		rs.Present();
	};

	factory.Run();

	return 0;
}
