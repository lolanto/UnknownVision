#include "D3D12/DX12RenderBasic.h"
#include "Utility/WindowBase/win32/WindowWin32.h"
#include <iostream>
#include <random>
#include <stack>

using namespace UnknownVision;

float triangleVtxs[] = {
	-0.5f, 0.0f, 0.0f,
	0.0f, 0.5f, 0.0f,
	0.5f, 0.0f, 0.0f
};

float test[9] = { 0 };

int main() {
	WindowWin32 win("test", 480, 320, true);
	win.Init();
	DX12RenderBackend rb;
	rb.Initialize();
	DX12BackendUsedData bud = { &win, win.Width(), win.Height() };
	auto rd = reinterpret_cast<DX12RenderDevice*>(rb.CreateDevice(&bud));
	rd->Initialize("");
	CommandUnit& cu = rd->RequestCommandUnit(DEFAULT_COMMAND_UNIT);

	win.SetMainLoopFuncPtr([&rb, &rd, &cu](float) {
		TextureHandle th0 = rd->RequestTexture(rd->CurrentBackBufferHandle());
		cu.Reset();
		cu.Active();
		cu.TransferState(th0, RESOURCE_STATE_RENDER_TARGET);
		cu.ClearRenderTarget(th0, { 0.2f, 0.4f, 0.8f, 0.0f });
		cu.TransferState(th0, RESOURCE_STATE_PRESENT);
		cu.FetchAndPresent();
	});
	win.Run();
	cu.Wait();
	return 0;
}
