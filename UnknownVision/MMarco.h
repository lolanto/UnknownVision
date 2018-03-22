#pragma once
class DXRenderer;
class MainClass;
class Material;
class Pipeline;

// 算法函数的默认参数
#define ALGDefualtParam DXRenderer* renderer, MainClass* mc

// 默认摄像机控制器的设置
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

// Renderer调用UnknownObject的setup时候的简化
// 必须配合 ALGDefualtParam 宏使用
// 依赖renderer变量
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
// 一个系列的shader必须有统一的命名规则 XXXPS / XXXVS / XXXGS
#define MatSetVP(x) MatSetVS(x##VS) MatSetPS(x##PS)
#define MatSetVGP(x) MatSetVS(x##VS) MatSetGS(x##GS) MatSetPS(x##PS)

// Shader Marco
#define ShaderPathPrefix std::string("../Debug/")
#define ShaderGenVS(sname, fname) VertexShader1 sname##VS(ShaderPathPrefix + std::string(#fname) + std::string("VS.cso"));
#define ShaderGenPS(sname, fname) PixelShader1 sname##PS(ShaderPathPrefix + std::string(#fname) + std::string("PS.cso"));
#define ShaderGenGS(sname, fname) GeometryShader1 sname##GS(ShaderPathPrefix + std::string(#fname) + std::string("GS.cso"));

// extract Shader Marco Helper
// 生成一个系列的VS+PS着色器
#define ShaderGenVP(sname, fname) \
ShaderGenVS(sname, fname) \
RendererSetup(sname##VS) \
ShaderGenPS(sname, fname) \
RendererSetup(sname##PS)

// 生成一个系列的VS+GS+PS着色器
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

// 为pipeline增加网格
#define PplAddMesh(x) _tempPipeline->AddMesh(x);
/* 为pipeline增加Render Target
x: Render Target
c: RT清空策略*/
#define PplAddRT(x, c) _tempPipeline->AddRenderTarget(&x, c);
/* 为pipeline增加Depth Stencil Buffer，不添加默认不启动深度和模版测试
x: Depth Stencil Buffer
c: DS 清空策略*/
#define PplSetDS(x, c) _tempPipeline->SetDepthStencilView(x, c);

#define PplSetCam(x) _tempPipeline->SetCamera(&x);
#define PplSetMat(x) _tempPipeline->SetMaterial(&x);
#define PplSetup RendererSetup(*_tempPipeline);

// extract Pipeline helper marco
// Render target清空策略
// 绑定前，解除绑定后清空RT
#define Ppl_BC_AC true, true
// 绑定前不清空RT， 解除绑定后清空RT
#define Ppl_NBC_AC false, true
// 绑定前，解除绑定后不清空RT
#define Ppl_NBC_NAC false, false
// 绑定前清空RT， 解除绑定后不清空RT
#define Ppl_BC_NAC true, false