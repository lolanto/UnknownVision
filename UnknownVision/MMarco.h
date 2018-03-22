#pragma once
class DXRenderer;
class MainClass;
class Material;
class Pipeline;

// �㷨������Ĭ�ϲ���
#define ALGDefualtParam DXRenderer* renderer, MainClass* mc

// Ĭ�������������������
#define CameraControllerSetting(x) \
	MainClass::UserFunc = [&x](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->void { \
		switch (uMsg) { \
			case WM_MOUSEMOVE: \
				x.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); \
				break; \
			case WM_LBUTTONDOWN: \
				x.MouseEventHandler(MBTN_LEFT, true); \
				break; \
			case WM_LBUTTONUP: \
				x.MouseEventHandler(MBTN_LEFT, false); \
				break; \
			case WM_RBUTTONDOWN: \
				x.MouseEventHandler(MBTN_RIGHT, true); \
				break; \
			case WM_RBUTTONUP: \
				x.MouseEventHandler(MBTN_RIGHT, false); \
				break; \
			case WM_MOUSEWHEEL: \
				x.MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)); \
				break; \
		} \
	};

// Renderer����UnknownObject��setupʱ��ļ�
// ������� ALGDefualtParam ��ʹ��
// ����renderer����
#define RendererSetup(x) renderer->Setup(&x);
#define IMRunPipeline(x) renderer->Bind(&x); renderer->Unbind(&x);
#define MainDS renderer->GetMainDepthStencilBuffer()
#define MainRT renderer->GetMainRT()
#define SetRS(x) renderer->SetRenderState(&x);
#define EndFrame renderer->EndRender();

#ifndef _MaterialSetting
#define _MaterialSetting
Material* _tempMat = 0;
#define MaterialSettingBeg(x) _tempMat = &x;
#define MaterialSettingEnd _tempMat = 0;
#endif

#define MatSetPS(x) _tempMat->SetPixelShader(&x);
#define MatSetVS(x) _tempMat->SetVertexShader(&x);
#define MatSetGS(x) _tempMat->SetGeometryShader(&x);
#define MatSetSamplerState(sampler, target, slot) _tempMat->SetSamplerState(&sampler, target, slot);
#define MatSetTexture(texture, target, slot) _tempMat->SetTexture(&texture, target, slot);
#define MatSetup RendererSetup(*_tempMat);

// extract Mat Marco Helper
// һ��ϵ�е�shader������ͳһ���������� XXXPS / XXXVS / XXXGS
#define MatSetVP(x) MatSetVS(x##VS) MatSetPS(x##PS)
#define MatSetVGP(x) MatSetVS(x##VS) MatSetGS(x##GS) MatSetPS(x##PS)

// Shader Marco
#define ShaderPathPrefix std::string("../Debug/")
#define ShaderGenVS(sname, fname) VertexShader1 sname##VS(ShaderPathPrefix + std::string(#fname) + std::string("VS.cso"));
#define ShaderGenPS(sname, fname) PixelShader1 sname##PS(ShaderPathPrefix + std::string(#fname) + std::string("PS.cso"));
#define ShaderGenGS(sname, fname) GeometryShader1 sname##GS(ShaderPathPrefix + std::string(#fname) + std::string("GS.cso"));

// extract Shader Marco Helper
// ����һ��ϵ�е�VS+PS��ɫ��
#define ShaderGenVP(sname, fname) \
ShaderGenVS(sname, fname) \
RendererSetup(sname##VS) \
ShaderGenPS(sname, fname) \
RendererSetup(sname##PS)

// ����һ��ϵ�е�VS+GS+PS��ɫ��
#define ShaderGenVGP(sname, fname) \
ShaderGenVS(sname, fname) \
RendererSetup(sname##VS) \
ShaderGenGS(sname, fname) \
RendererSetup(sname##GS) \
ShaderGenPS(sname, fname) \
RendererSetup(sname##PS)

// Pipeline Marco
#ifndef _PipelineSetting
#define _PipelineSetting
Pipeline* _tempPipeline = 0;
#define PipelineSettingBeg(x) _tempPipeline = &x;
#define PipelineSettingEnd _tempPipeline = 0;
#endif

// Ϊpipeline��������
#define PplAddMesh(x) _tempPipeline->AddMesh(x);
/* Ϊpipeline����Render Target
x: Render Target
c: RT��ղ���*/
#define PplAddRT(x, c) _tempPipeline->AddRenderTarget(&x, c);
/* Ϊpipeline����Depth Stencil Buffer�������Ĭ�ϲ�������Ⱥ�ģ�����
x: Depth Stencil Buffer
c: DS ��ղ���*/
#define PplSetDS(x, c) _tempPipeline->SetDepthStencilView(x, c);

#define PplSetCam(x) _tempPipeline->SetCamera(&x);
#define PplSetMat(x) _tempPipeline->SetMaterial(&x);
#define PplSetup RendererSetup(*_tempPipeline);

// extract Pipeline helper marco
// Render target��ղ���
// ��ǰ������󶨺����RT
#define Ppl_BC_AC true, true
// ��ǰ�����RT�� ����󶨺����RT
#define Ppl_NBC_AC false, true
// ��ǰ������󶨺����RT
#define Ppl_NBC_NAC false, false
// ��ǰ���RT�� ����󶨺����RT
#define Ppl_BC_NAC true, false