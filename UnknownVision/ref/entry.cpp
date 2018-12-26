//#include <windowsx.h>
//#include <DirectXMath.h>
//#include "MainClass.h"
//#include "InfoLog.h"
//
//#include "DXRenderer.h"
//#include "Alg.h"
//
//using std::cout;
//using std::endl;
//
//const float CUBE_MAP_SIZE = 1280.0f;
//
//typedef DirectX::XMFLOAT4 float4;
//
//void main() {
//	// Ϊ���ܹ�ʹ��DXTex�⣬��Ҫ�ȳ�ʼ��Com�����
//	CoInitialize(NULL);
//	MainClass mc("UnknownVision");
//	if (FAILED(mc.CreateDesktopWindow(WIDTH, HEIGHT))) {
//		MLOG(LE, "Created Window Failed!");
//		system("pause");
//		return;
//	}
//	if (!DXRenderer::GetInstance().InitSys(mc.GetWindowHandle(), WIDTH, HEIGHT)) {
//		// ��Ⱦ����ʼ��ʧ��
//		MLOG(LE, "Renderer Initialize Failed!");
//		system("pause");
//		return;
//	}
//
//	UITest(&DXRenderer::GetInstance(), &mc);
//	//LTC(&DXRenderer::GetInstance(), &mc);
//	
//	CoUninitialize();
//	system("pause");
//}

#include "ResMgr/SmartPtr.h"
#include <iostream>
using namespace std;
template<>
SmartPtr<int> Make(int* ptr) {
	SmartPtr<int> ret;
	ret._ptr = ptr;
	ret._count = new size_t(1);
	return ret;
}

void main() {
	SmartPtr<int> s;
	s = Make(new int(2));
	cout << s.GetCount() << endl;
}