#include <windowsx.h>
#include <DirectXMath.h>
#include "MainClass.h"
#include "InfoLog.h"

#include "DXRenderer.h"
#include "Alg.h"

using std::cout;
using std::endl;

const float CUBE_MAP_SIZE = 1280.0f;

typedef DirectX::XMFLOAT4 float4;

void main() {
	// 为了能够使用DXTex库，需要先初始化Com组件库
	CoInitialize(NULL);
	MainClass mc("UnknownVision");
	if (FAILED(mc.CreateDesktopWindow(WIDTH, HEIGHT))) {
		MLOG(LE, "Created Window Failed!");
		system("pause");
		return;
	}
	if (!DXRenderer::GetInstance().InitSys(mc.GetWindowHandle(), WIDTH, HEIGHT)) {
		// 渲染器初始化失败
		MLOG(LE, "Renderer Initialize Failed!");
		system("pause");
		return;
	}

	UITest(&DXRenderer::GetInstance(), &mc);
	//LTC(&DXRenderer::GetInstance(), &mc);
	
	CoUninitialize();
	system("pause");
}