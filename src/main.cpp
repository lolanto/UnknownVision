#include "Utility/WindowBase/win32/WindowWin32.h"
#include "D3D12\DX12RenderBackend.h"
#include "D3D12\DX12RenderDevice.h"
#include "D3D12\DX12Config.h"
#include "D3D12\GPUResource\DX12StaticVertexBuffer.h"
#include "D3D12\GPUResource\DX12StaticIndexBuffer.h"
#include "Shader\SampleShader.h"

#include <DirectXMath.h>
#include <iostream>
#include <random>
#include <stack>

using namespace UnknownVision;

float triangleVtxs[] = {
	-0.5f, 0.0f, 0.0f,
	0.0f, 0.5f, 0.0f,
	0.5f, 0.0f, 0.0f
};
float BLUE[] = { 0.2f, 0.4f, 0.6f, 1.0f };
uint32_t indices[] = { 0, 1, 2 };

constexpr uint32_t gWidth = 960;
constexpr uint32_t gHeight = 480;

int main() {
	WindowWin32 win("test", gWidth, gHeight, true);
	win.Init();
	DX12RenderBackend backend;
	DX12BackendUsedData bkData = { &win, gWidth, gHeight };
	assert(backend.Initialize());
	DX12RenderDevice* pDevice = reinterpret_cast<DX12RenderDevice*>(backend.CreateDevice(&bkData));
	assert(pDevice->Initialize(""));
	
	SampleShaderVS svs;
	SampleShaderPS sps;
	assert(backend.InitializeShaderObject(&svs));
	assert(backend.InitializeShaderObject(&sps));
	auto pPipeline = pDevice->BuildGraphicsPipelineObject(&svs, &sps,
		GDefaultRasterizeOptions, GDefaultOutputStageOptions, SampleVertexAttributes);
	assert(pPipeline != nullptr);
	
	ViewPort vp;
	vp.topLeftX = vp.topLeftY = 0;
	vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
	vp.width = gWidth; vp.height = gHeight;
	ScissorRect sr;
	sr.top = sr.left = 0; sr.right = gWidth; sr.bottom = gHeight;

	CommandUnit* cmdUnit = pDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	DX12StaticVertexBuffer<DirectX::XMFLOAT3> vertexBuffer;
	vertexBuffer.SetInitialData(3, triangleVtxs);
	vertexBuffer.RequestPermenent(cmdUnit);
	vertexBuffer.SetName(L"VertexBuffer");
	DX12StaticIndexBuffer indexBuffer;
	indexBuffer.SetInitialData(3, indices);
	indexBuffer.RequestPermenent(cmdUnit);
	indexBuffer.SetName(L"IndexBuffer");

	cmdUnit->TransferState(&vertexBuffer, RESOURCE_STATE_VERTEX_BUFFER);
	cmdUnit->TransferState(&indexBuffer, RESOURCE_STATE_INDEX_BUFFER);

	win.MainLoop = [&cmdUnit, &pPipeline, &pDevice, &vp, &sr, &vertexBuffer, &indexBuffer](float deltaTime) {
		GPUResource* vbs[] = { &vertexBuffer };
		GPUResource* rts[] = { pDevice->BackBuffer() };
		cmdUnit->TransferState(pDevice->BackBuffer(), RESOURCE_STATE_RENDER_TARGET);
		cmdUnit->ClearRenderTarget(pDevice->BackBuffer(), BLUE);
		cmdUnit->BindPipeline(pPipeline);
		cmdUnit->BindViewports(1, &vp);
		cmdUnit->BindScissorRects(1, &sr);
		cmdUnit->BindVertexBuffers(0, 1, vbs);
		cmdUnit->BindIndexBuffer(&indexBuffer);
		cmdUnit->BindRenderTargets(rts, 1, nullptr);
		cmdUnit->Draw(0, 3, 0);
		cmdUnit->TransferState(pDevice->BackBuffer(), RESOURCE_STATE_PRESENT);
		cmdUnit->Flush(false);
		cmdUnit->Present();
		pDevice->UpdatePerFrame();
	};
	win.Run();
	return 0;
}
