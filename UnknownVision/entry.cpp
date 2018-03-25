#include <windowsx.h>
#include <DirectXMath.h>
#include "MainClass.h"
#include "InfoLog.h"

#include "DXRenderer.h"
#include "Alg.h"

using std::cout;
using std::endl;

const float WIDTH = 1280.0f;
const float HEIGHT = 960.0f;

const float CUBE_MAP_SIZE = 1280.0f;

typedef DirectX::XMFLOAT4 float4;

void main() {
	// Ϊ���ܹ�ʹ��DXTex�⣬��Ҫ�ȳ�ʼ��Com�����
	CoInitialize(NULL);
	MainClass mc("UnknownVision");
	if (FAILED(mc.CreateDesktopWindow(WIDTH, HEIGHT))) {
		MLOG(LE, "Created Window Failed!");
		system("pause");
		return;
	}
	if (!DXRenderer::GetInstance().InitSys(mc.GetWindowHandle(), WIDTH, HEIGHT)) {
		// ��Ⱦ����ʼ��ʧ��
		MLOG(LE, "Renderer Initialize Failed!");
		system("pause");
		return;
	}

	ImageBasedLighting(&DXRenderer::GetInstance(), &mc);
	
	CoUninitialize();
	system("pause");
}