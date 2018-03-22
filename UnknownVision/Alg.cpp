#include "Alg.h"
#include "MMarco.h"
#include <windowsx.h>
#include "MainClass.h"
#include "InfoLog.h"

#include "DXRenderer.h"
#include "Model.h"
#include "Camera.h"
#include "Canvas.h"
#include "Light.h"
#include "Shader.h"
#include "Material.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "Sampler.h"
#include "RasterState.h"
#include "UI.h"

const float WIDTH = 1280.0f;
const float HEIGHT = 960.0f;

const float CUBE_MAP_SIZE = 1280.0f;

typedef DirectX::XMFLOAT4 float4;

Sampler linearSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
Sampler pointSampler(D3D11_FILTER_MIN_MAG_MIP_POINT);

void CubeMapGen(DXRenderer* renderer, MainClass* mc) {
	RasterState normalRS;
	renderer->Setup(&normalRS);
	RasterState cullFrontFace(D3D11_CULL_FRONT);
	renderer->Setup(&cullFrontFace);

	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 0.0f, -0.01f);
	Camera cc(camDesc);
	renderer->Setup(&cc);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	MainClass::UserFunc = [&obController](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->void {
		switch (uMsg) {
		case WM_MOUSEMOVE:
			obController.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_LBUTTONDOWN:
			obController.MouseEventHandler(MBTN_LEFT, true);
			break;
		case WM_LBUTTONUP:
			obController.MouseEventHandler(MBTN_LEFT, false);
			break;
		case WM_RBUTTONDOWN:
			obController.MouseEventHandler(MBTN_RIGHT, true);
			break;
		case WM_RBUTTONUP:
			obController.MouseEventHandler(MBTN_RIGHT, false);
			break;
		case WM_MOUSEWHEEL:
			obController.MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			break;
		}
	};

	// cubemap viewport
	D3D11_VIEWPORT cubemapViewport = {
		0.0f , 0.0f , CUBE_MAP_SIZE, CUBE_MAP_SIZE, 0.0f, 1.0f };

	// 创建shader
	VertexShader1 quadVS("../Debug/GenCubeMapQuadVertexShader.cso");
	renderer->Setup(&quadVS);

	VertexShader1 simpleVS("../Debug/GenCubeMapSimpleVertexShader.cso");
	renderer->Setup(&simpleVS);
	PixelShader1 simplePS("../Debug/GenCubeMapSimplePixelShader.cso");
	renderer->Setup(&simplePS);
	GeometryShader1 cubeGS("../Debug/GenCubeMapSimpleGeometryShader.cso");
	renderer->Setup(&cubeGS);

	PixelShader1 cubeReadPS("../Debug/GenCubeMapCubeMapReadPixelShader.cso");
	renderer->Setup(&cubeReadPS);

	VertexShader1 sceneVS("../Debug/GenCubeMapSceneVS.cso");
	renderer->Setup(&sceneVS);
	PixelShader1 scenePS("../Debug/GenCubeMapScenePS.cso");
	renderer->Setup(&scenePS);

	// 创建texture
	CommonTexture BC(L"./CubeMapScene/BC.png");
	renderer->Setup(&BC);

	// 创建canvas
	CanvasCubeMap cubeMap(CUBE_MAP_SIZE);
	renderer->Setup(&cubeMap);

	Canvas quadCanvas(WIDTH, HEIGHT);
	renderer->Setup(&quadCanvas);

	DepthTexture cubeDepthTexture(CUBE_MAP_SIZE, CUBE_MAP_SIZE);
	cubeDepthTexture.SetCubeMap();
	cubeDepthTexture.SetMipMap();
	renderer->Setup(&cubeDepthTexture);

	DepthTexture quadDepthTexture(WIDTH, HEIGHT);
	renderer->Setup(&quadDepthTexture);


	// 创建sampler
	Sampler comSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	renderer->Setup(&comSampler);
	Sampler pointSampler(D3D11_FILTER_MIN_MAG_MIP_POINT);
	renderer->Setup(&pointSampler);

	// 创建constant buffer
	ConstantBuffer cubeMapCamBuffer(cubeGS.GetShaderCBuffer(0));
	renderer->Setup(&cubeMapCamBuffer);
	ConstantBuffer cubeMapViewBuffer(simpleVS.GetShaderCBuffer(1));
	renderer->Setup(&cubeMapViewBuffer);

	DirectX::XMFLOAT4X4 cubeMapCamData[7];
	DirectX::XMFLOAT4X4 cubeMapViewData;

	CubeMapHelper cubeMapHelper({ 0, 3, 0 });
	cubeMapHelper.GetBasicViewMat(&cubeMapViewData);
	cubeMapHelper.GetRotAndProjMat(cubeMapCamData);

	cubeMapCamBuffer.Update("leftViewMat", cubeMapCamData, sizeof(DirectX::XMFLOAT4X4));
	cubeMapCamBuffer.Update("rightViewMat", cubeMapCamData + 1, sizeof(DirectX::XMFLOAT4X4));
	cubeMapCamBuffer.Update("topViewMat", cubeMapCamData + 2, sizeof(DirectX::XMFLOAT4X4));
	cubeMapCamBuffer.Update("bottomViewMat", cubeMapCamData + 3, sizeof(DirectX::XMFLOAT4X4));
	cubeMapCamBuffer.Update("forwardViewMat", cubeMapCamData + 4, sizeof(DirectX::XMFLOAT4X4));
	cubeMapCamBuffer.Update("backwardViewMat", cubeMapCamData + 5, sizeof(DirectX::XMFLOAT4X4));
	cubeMapCamBuffer.Update("projMatrix", cubeMapCamData + 6, sizeof(DirectX::XMFLOAT4X4));

	cubeMapViewBuffer.Update("basicViewMat", &cubeMapViewData, sizeof(DirectX::XMFLOAT4X4));

	ConstantBuffer cubeMapDataBuffer(scenePS.GetShaderCBuffer(1));
	renderer->Setup(&cubeMapDataBuffer);
	cubeMapDataBuffer.Update("CM_Matrix", &cubeMapViewData, sizeof(DirectX::XMFLOAT4X4));

	// 创建material
	Material simpleMat;
	simpleMat.SetVertexShader(&simpleVS);
	simpleMat.SetPixelShader(&simplePS);
	simpleMat.SetGeometryShader(&cubeGS);
	simpleMat.SetTexture(&BC, SBT_PIXEL_SHADER, 0);
	simpleMat.SetConstantBuffer(&cubeMapViewBuffer, SBT_VERTEX_SHADER, 3);
	simpleMat.SetConstantBuffer(&cubeMapCamBuffer, SBT_GEOMETRY_SHADER, 0);
	simpleMat.SetSamplerState(&comSampler, SBT_PIXEL_SHADER, 0);
	renderer->Setup(&simpleMat);

	Material sceneMat;
	sceneMat.SetVertexShader(&sceneVS);
	sceneMat.SetPixelShader(&scenePS);
	sceneMat.SetTexture(&cubeMap, SBT_PIXEL_SHADER, 0);
	sceneMat.SetTexture(&BC, SBT_PIXEL_SHADER, 1);
	sceneMat.SetConstantBuffer(&cubeMapDataBuffer, SBT_PIXEL_SHADER, 1);
	sceneMat.SetSamplerState(&comSampler, SBT_PIXEL_SHADER, 0);
	renderer->Setup(&sceneMat);

	// 创建model
	Model m1;
	renderer->Setup(&m1);

	// 创建mesh
	MeshFactory mf;
	/*std::vector<std::shared_ptr<Mesh>> meshList = mf.Load("./PreviewSphere/previewSphere.obj");*/
	std::vector<std::shared_ptr<Mesh>> meshList = mf.Load("./CubeMapScene/testEnvSimple.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		renderer->Setup(iter._Ptr->get());
		iter._Ptr->get()->SetModel(&m1);
	}

	std::shared_ptr<Mesh> plane;
	mf.Load(BMT_PLANE, plane, 2, 2);
	plane->SetModel(&m1);
	renderer->Setup(plane.get());

	// 创建pipeline
	Pipeline cubeMapPL;
	cubeMapPL.SetMaterial(&simpleMat);
	cubeMapPL.AddMesh(meshList[0].get());
	cubeMapPL.SetDepthStencilView(cubeDepthTexture.GetDSV(), true, false);
	cubeMapPL.AddRenderTarget(&cubeMap, true, false);
	cubeMapPL.SetCamera(&cc);
	renderer->Setup(&cubeMapPL);

	Pipeline renderScene;
	renderScene.SetMaterial(&sceneMat);
	renderScene.AddMesh(meshList[0].get());
	renderScene.AddRenderTarget(renderer->GetMainRT(), true, false);
	renderScene.SetDepthStencilView(quadDepthTexture.GetDSV(), true, false);
	renderScene.SetCamera(&cc);
	renderer->Setup(&renderScene);

	UIRenderer::GetInstance().Prepare();

	mc->Run([&] {
		renderer->IterateFuncs();

		renderer->SetRenderState(&normalRS, &cubemapViewport);
		renderer->Bind(&cubeMapPL);
		renderer->Unbind(&cubeMapPL);
		renderer->SetRenderState(&normalRS);
		renderer->Bind(&renderScene);
		renderer->Unbind(&renderScene);
		renderer->EndRender();
	});
}

void DeepGBuffer(ALGDefualtParam) {
	RasterState normalRS;
	RendererSetup(normalRS)
	MeshFactory mf;
	// 创建camera desc
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 0.0f, -0.01f);
	Camera cc(camDesc);
	RendererSetup(cc);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController)

	// 创建深度图以及渲染对象
	Canvas canvasArray1(WIDTH, HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, false, 2);
	RendererSetup(canvasArray1)
	Canvas canvasArray2(WIDTH, HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, false, 2);
	renderer->Setup(&canvasArray2);
	DepthTexture depthTexArray1(WIDTH, HEIGHT, 2);
	renderer->Setup(&depthTexArray1);
	DepthTexture depthTexArray2(WIDTH, HEIGHT, 2);
	renderer->Setup(&depthTexArray2);

	// 创建sampler
	Sampler pointSampler(D3D11_FILTER_MIN_MAG_MIP_POINT);
	renderer->Setup(&pointSampler);
	Sampler comSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	renderer->Setup(&comSampler);

	// 创建基础贴图
	CommonTexture BC(L"./CubeMapScene/BC.png");
	renderer->Setup(&BC);

	// 加载模型
	Model basicModel;
	renderer->Setup(&basicModel);
	// 加载网格
	std::vector<std::shared_ptr<Mesh>> meshList = mf.Load("./CubeMapScene/testEnv.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		renderer->Setup(iter._Ptr->get());
		iter->get()->SetModel(&basicModel);
	}

	std::shared_ptr<Mesh> plane;
	mf.Load(BMT_PLANE, plane, 2, 2);
	plane->SetModel(&basicModel);
	renderer->Setup(plane.get());

	// 创建shader
	ShaderGenVGP(DGBuf, DeepGBuffer);
	ShaderGenVP(quad, DeepGBufferQuad);

	// 创建material
	Material DGMat;
	MaterialSettingBeg(DGMat);
	MatSetVGP(DGBuf);
	MatSetSamplerState(pointSampler, SBT_PIXEL_SHADER, 0);
	MatSetTexture(BC, SBT_PIXEL_SHADER, 0);
	MatSetup;
	MaterialSettingEnd;


	Material quadMat;
	MaterialSettingBeg(quadMat);
	MatSetVP(quad);
	MatSetSamplerState(comSampler, SBT_PIXEL_SHADER, 0);
	MatSetTexture(canvasArray1, SBT_PIXEL_SHADER, 0);
	MatSetup;
	MaterialSettingEnd;

	// 创建pipeline
	Pipeline DGPipeline;
	PipelineSettingBeg(DGPipeline);
	PplAddMesh(meshList[0].get());
	PplSetCam(cc);
	PplSetMat(DGMat);
	PplSetup;
	PipelineSettingEnd;

	Pipeline quadPipeline;
	PipelineSettingBeg(quadPipeline);
	PplAddMesh(plane.get());
	PplAddRT(*renderer->GetMainRT(), Ppl_BC_NAC);
	PplSetCam(cc);
	PplSetMat(quadMat);
	PplSetup;
	PipelineSettingEnd;

	mc->Run([&] {
		renderer->SetRenderState(&normalRS);

		DGPipeline.AddRenderTarget(&canvasArray1, true, false);
		DGMat.SetTexture(&depthTexArray2, SBT_PIXEL_SHADER, 1);
		DGPipeline.SetDepthStencilView(depthTexArray1.GetDSV(), true, false);
		renderer->Bind(&DGPipeline);
		renderer->Unbind(&DGPipeline);

		DGPipeline.RemoveRenderTarget(&canvasArray1);

		DGPipeline.AddRenderTarget(&canvasArray2, true, false);
		DGMat.SetTexture(&depthTexArray1, SBT_PIXEL_SHADER, 1);
		DGPipeline.SetDepthStencilView(depthTexArray2.GetDSV(), true, false);
		renderer->Bind(&DGPipeline);
		renderer->Unbind(&DGPipeline);

		DGPipeline.RemoveRenderTarget(&canvasArray2);

		renderer->Bind(&quadPipeline);
		renderer->Unbind(&quadPipeline);
		renderer->EndRender();
	});
}

void ScreenSpaceReflection(DXRenderer* renderer, MainClass* mc) {
	RasterState normalRS;
	renderer->Setup(&normalRS);
	MeshFactory mf;
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 0.0f, -0.01f);
	Camera cc(camDesc);
	renderer->Setup(&cc);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	MainClass::UserFunc = [&obController](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->void {
		switch (uMsg) {
		case WM_MOUSEMOVE:
			obController.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_LBUTTONDOWN:
			obController.MouseEventHandler(MBTN_LEFT, true);
			break;
		case WM_LBUTTONUP:
			obController.MouseEventHandler(MBTN_LEFT, false);
			break;
		case WM_RBUTTONDOWN:
			obController.MouseEventHandler(MBTN_RIGHT, true);
			break;
		case WM_RBUTTONUP:
			obController.MouseEventHandler(MBTN_RIGHT, false);
			break;
		case WM_MOUSEWHEEL:
			obController.MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			break;
		}
	};

	// 创建GBuffer资源--颜色信息以及法线信息
	Canvas ssColor(WIDTH, HEIGHT);
	Canvas ssNormalAndLinearZ(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	renderer->Setup(&ssColor);
	renderer->Setup(&ssNormalAndLinearZ);
	Canvas result1(WIDTH, HEIGHT);
	Canvas result2(WIDTH, HEIGHT);
	renderer->Setup(&result1);
	renderer->Setup(&result2);

	// 创建普通深度图
	DepthTexture depthTex(WIDTH, HEIGHT);
	renderer->Setup(&depthTex);
	// model
	Model m1;
	renderer->Setup(&m1);
	// 加载模型
	std::vector<std::shared_ptr<Mesh>> meshList = mf.Load("./CubeMapScene/testEnv.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		renderer->Setup(iter->get());
		iter->get()->SetModel(&m1);
	}
	// 加载平面
	std::shared_ptr<Mesh> plane;
	mf.Load(BMT_PLANE, plane, 2, 2);
	renderer->Setup(plane.get());
	plane->SetModel(&m1);

	// 基本贴图
	CommonTexture BC(L"./CubeMapScene/BC.png");
	renderer->Setup(&BC);
	// 创建sampler
	Sampler pointSampler(D3D11_FILTER_MIN_MAG_MIP_POINT);
	renderer->Setup(&pointSampler);
	Sampler linearSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	renderer->Setup(&linearSampler);

	// 创建shader
	PixelShader1 SSRPS("../Debug/ScreenSpaceReflectionPS.cso");
	renderer->Setup(&SSRPS);
	VertexShader1 SSRVS("../Debug/ScreenSpaceReflectionVS.cso");
	renderer->Setup(&SSRVS);
	PixelShader1 SSRQuadPS("../Debug/ScreenSpaceReflectionQuadPS.cso");
	renderer->Setup(&SSRQuadPS);
	VertexShader1 SSRQuadVS("../Debug/ScreenSpaceReflectionQuadVS.cso");
	renderer->Setup(&SSRQuadVS);

	// 创建material
	Material SSRMat;
	SSRMat.SetVertexShader(&SSRVS);
	SSRMat.SetPixelShader(&SSRPS);
	SSRMat.SetTexture(&BC, SBT_PIXEL_SHADER, 0);
	SSRMat.SetSamplerState(&linearSampler, SBT_PIXEL_SHADER, 0);
	renderer->Setup(&SSRMat);

	Material SSRQuadMat;
	SSRQuadMat.SetVertexShader(&SSRQuadVS);
	SSRQuadMat.SetPixelShader(&SSRQuadPS);
	SSRQuadMat.SetTexture(&ssNormalAndLinearZ, SBT_PIXEL_SHADER, 0);
	SSRQuadMat.SetTexture(&ssColor, SBT_PIXEL_SHADER, 1);
	SSRQuadMat.SetSamplerState(&pointSampler, SBT_PIXEL_SHADER, 0);
	SSRQuadMat.SetSamplerState(&linearSampler, SBT_PIXEL_SHADER, 1);

	// 创建pipeline
	Pipeline SSRPipeline;
	SSRPipeline.AddMesh(meshList[0].get());
	SSRPipeline.SetMaterial(&SSRMat);
	SSRPipeline.AddRenderTarget(&ssColor, true, false);
	SSRPipeline.AddRenderTarget(&ssNormalAndLinearZ, true, false);
	SSRPipeline.SetDepthStencilView(depthTex.GetDSV(), true, false);
	SSRPipeline.SetCamera(&cc);

	Pipeline quadPipeline;
	quadPipeline.SetMaterial(&SSRQuadMat);
	quadPipeline.AddMesh(plane.get());
	quadPipeline.AddRenderTarget(renderer->GetMainRT(), true, false);
	quadPipeline.SetCamera(&cc);

	mc->Run([&] {
		renderer->SetRenderState(&normalRS);
		renderer->Bind(&SSRPipeline);
		renderer->Unbind(&SSRPipeline);
		renderer->Bind(&quadPipeline);
		renderer->Unbind(&quadPipeline);
		renderer->EndRender();
	});
}

void ImageBasedLighting(DXRenderer* renderer, MainClass* mc) {
	RasterState normalRS;
	renderer->Setup(&normalRS);
	MeshFactory mf;
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 8.0f, -3.0f);
	Camera cc(camDesc);
	renderer->Setup(&cc);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	MainClass::UserFunc = [&obController](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->void {
		switch (uMsg) {
		case WM_MOUSEMOVE:
			obController.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_LBUTTONDOWN:
			obController.MouseEventHandler(MBTN_LEFT, true);
			break;
		case WM_LBUTTONUP:
			obController.MouseEventHandler(MBTN_LEFT, false);
			break;
		case WM_RBUTTONDOWN:
			obController.MouseEventHandler(MBTN_RIGHT, true);
			break;
		case WM_RBUTTONUP:
			obController.MouseEventHandler(MBTN_RIGHT, false);
			break;
		case WM_MOUSEWHEEL:
			obController.MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			break;
		}
	};

	// 创建模型
	Model m1;
	renderer->Setup(&m1);
	Model m2;
	renderer->Setup(&m2);
	DirectX::XMFLOAT3 planePos = { 2.0f, 4.0f, 2.0f };
	DirectX::XMFLOAT3 planeRotate = { 0.0f, 0.74f, 0.0f };
	m2.RotateAroundOrigin(planeRotate);
	m2.Translate(planePos);
	// 加载网格
	std::vector<std::shared_ptr<Mesh>> meshList = mf.Load("./ImageBasedLighting/testEnv.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		renderer->Setup(iter._Ptr->get());
		iter->get()->SetModel(&m1);
	}
	std::shared_ptr<Mesh> plane;
	mf.Load(BMT_PLANE, plane, 4.0f, 4.0f);
	renderer->Setup(plane.get());
	plane->SetModel(&m2);

	// depth texture
	DepthTexture mainDepth(WIDTH, HEIGHT);
	renderer->Setup(&mainDepth);

	// Shader
	VertexShader1 IBLVS("../Debug/imageBasedLightingVS.cso");
	renderer->Setup(&IBLVS);
	PixelShader1 IBLPS("../Debug/imageBasedLightingPS.cso");
	renderer->Setup(&IBLPS);

	VertexShader1 IBLBasicVS("../Debug/imageBasedLightingBasicVS.cso");
	renderer->Setup(&IBLBasicVS);
	PixelShader1 IBLBasicPS("../Debug/imageBasedLightingBasicPS.cso");
	renderer->Setup(&IBLBasicPS);

	// sampler
	Sampler linearSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	renderer->Setup(&linearSampler);

	// texture
	CommonTexture BC(L"./ImageBasedLighting/AO.png");
	renderer->Setup(&BC);
	CommonTexture BoardColor(L"./ImageBasedLighting/based.jpg");
	renderer->Setup(&BoardColor);

	// constant buffer
	ConstantBuffer IBLVSConstantBuf(IBLVS.GetShaderCBuffer(2));
	renderer->Setup(&IBLVSConstantBuf);
	ConstantBuffer IBLPSConstantBuf(IBLPS.GetShaderCBuffer(0));
	renderer->Setup(&IBLPSConstantBuf);

	// material
	Material IBLMat;
	IBLMat.SetVertexShader(&IBLVS);
	IBLMat.SetPixelShader(&IBLPS);
	IBLMat.SetTexture(&BC, SBT_PIXEL_SHADER, 0);
	IBLMat.SetTexture(&BoardColor, SBT_PIXEL_SHADER, 1);
	IBLMat.SetSamplerState(&linearSampler, SBT_PIXEL_SHADER, 0);
	IBLMat.SetConstantBuffer(&IBLVSConstantBuf, SBT_VERTEX_SHADER, 3);
	IBLMat.SetConstantBuffer(&IBLPSConstantBuf, SBT_PIXEL_SHADER, 1);
	renderer->Setup(&IBLMat);

	Material IBLBoardMat;
	IBLBoardMat.SetVertexShader(&IBLBasicVS);
	IBLBoardMat.SetPixelShader(&IBLBasicPS);
	IBLBoardMat.SetTexture(&BoardColor, SBT_PIXEL_SHADER, 0);
	IBLBoardMat.SetSamplerState(&linearSampler, SBT_PIXEL_SHADER, 0);
	renderer->Setup(&IBLBoardMat);

	// pipeline
	Pipeline IBLPipeline;
	IBLPipeline.AddMesh(meshList[0].get());
	IBLPipeline.SetMaterial(&IBLMat);
	IBLPipeline.SetCamera(&cc);
	IBLPipeline.AddRenderTarget(renderer->GetMainRT(), true, false);
	IBLPipeline.SetDepthStencilView(mainDepth.GetDSV(), true, false);
	renderer->Setup(&IBLPipeline);

	Pipeline IBLBoardPipeline;
	IBLBoardPipeline.AddMesh(plane.get());
	IBLBoardPipeline.SetMaterial(&IBLBoardMat);
	IBLBoardPipeline.SetCamera(&cc);
	IBLBoardPipeline.AddRenderTarget(renderer->GetMainRT(), false, false);
	IBLBoardPipeline.SetDepthStencilView(mainDepth.GetDSV(), false, false);
	renderer->Setup(&IBLBoardPipeline);

	mc->Run([&] {
		renderer->SetRenderState(&normalRS);
		DirectX::XMFLOAT4X4 modelINV = m2.GetModelData().modelMatrixInv;
		DirectX::XMFLOAT4 modelWH = { 4.0f, 4.0f, 0.0f, 0.0f };
		IBLVSConstantBuf.Update("BoardMatrix", &modelINV, sizeof(DirectX::XMFLOAT4X4));
		IBLPSConstantBuf.Update("WH", &modelWH, sizeof(modelWH));
		renderer->Bind(&IBLPipeline);
		renderer->Unbind(&IBLPipeline);
		renderer->Bind(&IBLBoardPipeline);
		renderer->Unbind(&IBLBoardPipeline);
		renderer->EndRender();
	});
}

void BruteForce(ALGDefualtParam) {
	RasterState normalRS;
	RendererSetup(normalRS);
	MeshFactory mf;
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 8.0f, -3.0f);
	Camera cc(camDesc);
	RendererSetup(cc);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController);

	// 创建贴图
	CommonTexture BC(L"./SSRScene/BC.png");
	RendererSetup(BC);

	// Sampler
	RendererSetup(linearSampler);
	RendererSetup(pointSampler);

	const int SMSize = 20;
	D3D11_VIEWPORT SMViewport;
	SMViewport.Height = SMSize;
	SMViewport.Width = SMSize;
	SMViewport.MaxDepth = 1.0f;
	SMViewport.MinDepth = 0.0f;
	SMViewport.TopLeftX = 0;
	SMViewport.TopLeftY = 0;
	// 创建场景的G-Buffer
	Canvas SMPos(SMSize, SMSize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SMPos);
	Canvas SMNor(SMSize, SMSize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SMNor);
	Canvas SMAlbedo(SMSize, SMSize);
	RendererSetup(SMAlbedo);

	// 创建当前屏幕空间的G-Buffer
	Canvas SSPos(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SSPos);
	Canvas SSNor(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SSNor);
	Canvas SSRef(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SSRef);
	Canvas SSAlbedo(WIDTH, HEIGHT);
	RendererSetup(SSAlbedo);

	// 创建记录反射信息的G-Buffer
	Canvas RSPos(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RSPos.unorderAccess = true;
	RendererSetup(RSPos);
	Canvas RSNor(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RSNor.unorderAccess = true;
	RendererSetup(RSNor);
	Canvas RSAlbedo(WIDTH, HEIGHT);
	RSAlbedo.unorderAccess = true;
	RendererSetup(RSAlbedo);
	Canvas RSResult(WIDTH, HEIGHT);
	RSResult.unorderAccess = true;
	RendererSetup(RSResult);

	Model basic;
	renderer->Setup(&basic);

	// 创建模型
	std::shared_ptr<Mesh> plane;
	mf.Load(BMT_PLANE, plane, 2.0f, 2.0f);
	renderer->Setup(plane.get());
	plane->SetModel(&basic);
	
	std::vector<std::shared_ptr<Mesh>> meshList = mf.Load("./SSRScene/testScene.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		renderer->Setup(iter->get());
		iter->get()->SetModel(&basic);
	}

	
	// 创建shader
	// 创建Scene Map相关的shader
	ShaderGenVP(BFSM, BruteForceSceneMap);
	// 创建屏幕空间G-buffer相关的shader
	ShaderGenVP(BFSS, BruteForceSS);
	// 创建读取G-buffer相关的shader
	ShaderGenVP(BFQuad, BruteForceQuad);
	// 创建compute shader
	ComputeShader1 BFCS("../Debug/BruteForceReflectCS.cso");
	RendererSetup(BFCS);

	// 创建材质
	// 创建Scene Map相关的材质
	Material SMMat;
	MaterialSettingBeg(SMMat);
	MatSetVP(BFSM);
	MatSetTexture(BC, SBT_PIXEL_SHADER, 0);
	MatSetSamplerState(linearSampler, SBT_PIXEL_SHADER, 0);
	MaterialSettingEnd;

	// 创建屏幕空间G-Buffer相关材质
	Material SSMat;
	MaterialSettingBeg(SSMat);
	MatSetVP(BFSS);
	MatSetTexture(BC, SBT_PIXEL_SHADER, 0);
	MatSetSamplerState(linearSampler, SBT_PIXEL_SHADER, 0);
	MaterialSettingEnd;

	// 创建读取G-Buffer相关信息的材质
	Material QuadMat;
	MaterialSettingBeg(QuadMat);
	MatSetVP(BFQuad);
	MatSetTexture(SSAlbedo, SBT_PIXEL_SHADER, 0);
	MatSetSamplerState(pointSampler, SBT_PIXEL_SHADER, 0);
	MaterialSettingEnd;

	// 创建pipeline
	// 创建Scene Map相关的pipeline
	Pipeline SMPipeline;
	PipelineSettingBeg(SMPipeline);
	PplAddMesh(meshList[0].get());
	PplAddRT(SMPos, Ppl_BC_NAC);
	PplAddRT(SMNor, Ppl_BC_NAC);
	PplAddRT(SMAlbedo, Ppl_BC_NAC);
	PplSetMat(SMMat);
	PipelineSettingEnd;

	// 创建屏幕空间G-Buffer相关的pipeline
	Pipeline SSPipeline;
	PipelineSettingBeg(SSPipeline);
	PplAddMesh(meshList[0].get());
	PplSetCam(cc);
	PplSetMat(SSMat);
	PplAddRT(SSPos, Ppl_BC_NAC);
	PplAddRT(SSNor, Ppl_BC_NAC);
	PplAddRT(SSRef, Ppl_BC_NAC);
	PplAddRT(SSAlbedo, Ppl_BC_NAC);
	PplSetDS(MainDS, Ppl_BC_NAC);
	PipelineSettingEnd;

	// 创建读取G-Buffers相关的pipeline
	Pipeline QuadPipeline;
	PipelineSettingBeg(QuadPipeline);
	PplAddMesh(plane.get());
	PplAddRT(*MainRT, Ppl_BC_NAC);
	PplSetMat(QuadMat);
	PplSetCam(cc);
	PipelineSettingEnd;

	// 创建compute pipeline
	ComputePipeline cptPipeline(&BFCS, { 40, 30, 1 });
	cptPipeline.BindResource(&SSPos, 0);
	cptPipeline.BindResource(&SSRef, 1);
	cptPipeline.BindResource(&SMPos, 2);
	cptPipeline.BindResource(&SMAlbedo, 3);
	cptPipeline.BindUATarget(&RSAlbedo, 0);
	RendererSetup(cptPipeline);

	SetRS(normalRS);

	mc->Run([&] {
		renderer->SetRenderState(&normalRS, &SMViewport);
		IMRunPipeline(SMPipeline);
		SetRS(normalRS);
		IMRunPipeline(SSPipeline);
		IMRunPipeline(cptPipeline);
		IMRunPipeline(QuadPipeline);
		EndFrame;
	});

}

void MyAlg(ALGDefualtParam)
{
	RasterState normalRS;
	RendererSetup(normalRS);
	MeshFactory mf;
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 8.0f, -3.0f);
	Camera cc(camDesc);
	RendererSetup(cc);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController);

	const int SamplePointX = 10;
	const int SamplePointY = 10;
	const int SamplePoints = SamplePointX * SamplePointY;

	// 创建texture
	CommonTexture BC(L"./SSRScene/BC.png");
	RendererSetup(BC);

	// 创建模型和网格
	Model basicModel;
	RendererSetup(basicModel);
	std::shared_ptr<Mesh> plane;
	mf.Load(BMT_PLANE, plane, 2.0f, 2.0f);
	RendererSetup(*plane.get());
	plane->SetModel(&basicModel);
	
	std::vector<std::shared_ptr<Mesh>> meshList = mf.Load("./SSRScene/testScene.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		RendererSetup(*iter->get());
		iter->get()->SetModel(&basicModel);
	}

	// pass1 创建SS的G-Buffer需要的材料
	// 创建Canvas
	Canvas SSPos(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SSPos);
	Canvas SSNor(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SSNor);
	Canvas SSRef(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RendererSetup(SSRef);
	Canvas SSAlbedo(WIDTH, HEIGHT);
	RendererSetup(SSAlbedo);

	// 创建shader
	ShaderGenVP(SSBasic, MyAlgSS);

	// 创建Mat
	Material SSMat;
	MaterialSettingBeg(SSMat);
	MatSetVP(SSBasic);
	MatSetTexture(BC, SBT_PIXEL_SHADER, 0);
	MatSetSamplerState(linearSampler, SBT_PIXEL_SHADER, 0);
	MaterialSettingEnd;

	// 创建pipeline
	Pipeline SSPipeline;
	PipelineSettingBeg(SSPipeline);
	PplAddMesh(meshList[0].get());
	PplSetCam(cc);
	PplSetMat(SSMat);
	PplAddRT(SSPos, Ppl_BC_NAC);
	PplAddRT(SSNor, Ppl_BC_NAC);
	PplAddRT(SSRef, Ppl_BC_NAC);
	PplAddRT(SSAlbedo, Ppl_BC_NAC);
	PplSetDS(MainDS, Ppl_BC_NAC);
	PipelineSettingEnd;

	// pass2 创建构造采样点相关的材料
	// 创建记录
	StructuredBuffer VPLMatrixs(SamplePoints, sizeof(DirectX::XMFLOAT4X4), false);
}