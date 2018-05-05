#include "Alg.h"
#include "MMarco.h"
#include <windowsx.h>
#include "MainClass.h"
#include "InfoLog.h"

#include "DXRenderer.h"
#include "Model.h"

#include "Texture.h"
#include "Shader.h"
#include "Sampler.h"
#include "Camera.h"
#include "Pass.h"
#include "Canvas.h"

const float CUBE_MAP_SIZE = 1280.0f;

typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT2 float2;

Sampler gLinearSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
Sampler gPointSampler(D3D11_FILTER_MIN_MAG_MIP_POINT);
MeshFactory gMF;

void ImageBasedLighting(DefaultParameters) {
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 5.0f, -3.0f);
	Camera cc(camDesc);
	cc.Setup(MainDev);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController);

	gLinearSampler.Setup(MainDev);
	gPointSampler.Setup(MainDev);

	// Shaders
	VertexShader IBLVS("../Debug/imageBasedLightingVS.cso");
	IBLVS.Setup(MainDev);
	PixelShader IBLPS("../Debug/imageBasedLightingPS.cso");
	IBLPS.Setup(MainDev);

	VertexShader IBLBasicVS("../Debug/imageBasedLightingBasicVS.cso");
	IBLBasicVS.Setup(MainDev);
	PixelShader IBLBasicPS("../Debug/imageBasedLightingBasicPS.cso");
	IBLBasicPS.Setup(MainDev);

	// Model
	Model basicModel;
	basicModel.Setup(MainDev);

	Model iblModel;
	DirectX::XMFLOAT3 dir = { 0.0f, 2.0f, 3.0f };
	DirectX::XMFLOAT3 rag = { 0.0f, 0.2f, 0.0f };
	iblModel.Translate(dir);
	iblModel.RotateAroundOrigin(rag);
	iblModel.Setup(MainDev);
	
	struct iblMatrixStruct {
		DirectX::XMFLOAT4X4 matrix;
	};

	struct iblWidthHeight {
		DirectX::XMFLOAT4 wh;
	};

	ConstantBuffer<iblMatrixStruct>  iblMatrixCB;
	iblMatrixCB.GetData().matrix = iblModel.GetModelData().modelMatrixInv;
	iblMatrixCB.Setup(MainDev);
	ConstantBuffer<iblWidthHeight> iblWidthHeightCB;
	iblWidthHeightCB.GetData().wh = { 1.0f, 1.0f, 0.0f, 0.0f };
	iblWidthHeightCB.Setup(MainDev);

	// Mesh
	std::vector<std::shared_ptr<Mesh>> meshList;
	meshList = gMF.Load("./ImageBasedLighting/testEnv.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		iter->get()->Setup(MainDev);
	}

	std::shared_ptr<Mesh> plane;
	gMF.Load(BMT_PLANE, plane, 1.0f, 1.0f);
	plane.get()->Setup(MainDev);

	// texture
	CommonTexture BC(L"./ImageBasedLighting/AO.png");
	BC.Setup(MainDev);
	CommonTexture refImage(L"./ImageBasedLighting/based.jpg");
	refImage.Setup(MainDev);

	// pass0
	ShadingPass pass0(&IBLVS, &IBLPS);
	pass0.BindSource(meshList[0].get());

	pass0.BindSource(&basicModel, SBT_VERTEX_SHADER, 1);
	pass0.BindSource(&cc, SBT_VERTEX_SHADER, 2);
	pass0.BindSource(&iblMatrixCB, SBT_VERTEX_SHADER, 3);

	pass0.BindSource(&BC, SBT_PIXEL_SHADER, 0);
	pass0.BindSource(&refImage, SBT_PIXEL_SHADER, 1);
	pass0.BindSource(&iblWidthHeightCB, SBT_PIXEL_SHADER, 1);
	pass0.BindSource(&cc, SBT_PIXEL_SHADER, 0);
	pass0.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	pass0.BindSource(renderer->GetMainRT(), true, false);
	pass0.BindSource(renderer->GetMainDS(), true, false);

	// pass1
	ShadingPass pass1(&IBLBasicVS, &IBLBasicPS);
	pass1.BindSource(plane.get());

	pass1.BindSource(&iblModel, SBT_VERTEX_SHADER, 1);
	pass1.BindSource(&cc, SBT_VERTEX_SHADER, 2);
	
	pass1.BindSource(&refImage, SBT_PIXEL_SHADER, 0);
	pass1.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	pass1.BindSource(renderer->GetMainRT(), false, false);
	pass1.BindSource(renderer->GetMainDS(), false, false);

	mc->Run([&]{
		pass0.Run(MainDevCtx);
		pass0.End(MainDevCtx);
		pass1.Run(MainDevCtx);
		pass1.End(MainDevCtx);
		renderer->EndRender();
	});

}

void LTC(DefaultParameters) {
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 5.0f, -3.0f);
	Camera cc(camDesc);
	cc.Setup(MainDev);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController);

	Sampler mLinearSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
	mLinearSampler.Setup(MainDev);
	gLinearSampler.Setup(MainDev);

	// Shaders
	VertexShader floorVS("../Debug/imageBasedLightingRefVS.cso");
	floorVS.Setup(MainDev);
	PixelShader floorPS("../Debug/imageBasedLightingRefPS.cso");
	floorPS.Setup(MainDev);
	VertexShader imgPlaneVS("../Debug/imageBasedLightingBasicVS.cso");
	imgPlaneVS.Setup(MainDev);
	PixelShader imgPlanePS("../Debug/imageBasedLightingBasicPS.cso");
	imgPlanePS.Setup(MainDev);


	struct IMGRect {
		float4 center;
		float4 plane;
		float4 wh_hwh;
	}; 
	DirectX::XMFLOAT3 rot = { 1.57f, 0.0f, 0.0f };
	ConstantBuffer<IMGRect> rectData;
	rectData.GetData().center = { 0, 1.2f, 3, 1 };
	rectData.GetData().plane = { 0, 0, -1, 3 };
	rectData.GetData().wh_hwh = { 2, 2, 1, 1 };
	rectData.Setup(MainDev);

	// model
	Model floorModel;
	floorModel.RotateAroundOrigin(rot);
	floorModel.Setup(MainDev);

	rot = {
		rectData.ReadData().center.x,
		rectData.ReadData().center.y,
		rectData.ReadData().center.z };

	Model imgModel;
	imgModel.Translate(rot);
	imgModel.Setup(MainDev);


	// plane
	std::shared_ptr<Mesh> floor;
	gMF.Load(BMT_PLANE, floor, 15.0f, 15.0f);
	floor->Setup(MainDev);

	std::shared_ptr<Mesh> imgPlane;
	gMF.Load(BMT_PLANE, imgPlane, rectData.ReadData().wh_hwh.x, rectData.ReadData().wh_hwh.y);
	imgPlane->Setup(MainDev);

	// Texture
	CommonTexture ORM(L"./LTC/ORM.png");
	ORM.Setup(MainDev);
	CommonTexture Base(L"./LTC/BC.png");
	Base.Setup(MainDev);
	DDSTexture ltc_mat(L"./LTC/ltc_mat.dds");
	ltc_mat.Setup(MainDev);
	DDSTexture glass(L"./LTC/glass.dds");
	glass.Setup(MainDev);

	// pass0
	// render floor
	ShadingPass pass0(&floorVS, &floorPS);
	// vs res
	pass0
		.BindSource(floor.get())
		.BindSource(&floorModel, SBT_VERTEX_SHADER, 1)
		.BindSource(&cc, SBT_VERTEX_SHADER, 2);

	// ps res
	pass0
		.BindSource(renderer->GetMainRT(), true, false)
		.BindSource(renderer->GetMainDS(), true, false)
		.BindSource(&ltc_mat, SBT_PIXEL_SHADER, 0)
		.BindSource(&cc, SBT_PIXEL_SHADER, 0)
		.BindSource(&glass, SBT_PIXEL_SHADER, 1)
		.BindSource(&ORM, SBT_PIXEL_SHADER, 2)
		.BindSource(&Base, SBT_PIXEL_SHADER, 3)
		.BindSource(&mLinearSampler, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 1)
		.BindSource(&rectData, SBT_PIXEL_SHADER, 1);

	// pass1
	// render image plane
	ShadingPass pass1(&imgPlaneVS, &imgPlanePS);
	// vs res
	pass1
		.BindSource(imgPlane.get())
		.BindSource(&imgModel, SBT_VERTEX_SHADER, 1)
		.BindSource(&cc, SBT_VERTEX_SHADER, 2);

	// ps res
	pass1
		.BindSource(renderer->GetMainRT(), false, false)
		.BindSource(renderer->GetMainDS(), false, false)
		.BindSource(&glass, SBT_PIXEL_SHADER, 0)
		.BindSource(&cc, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	mc->Run([&] {
		pass0.Run(MainDevCtx).End(MainDevCtx);
		pass1.Run(MainDevCtx).End(MainDevCtx);
		renderer->EndRender();
	});
}

void ScreenSpaceRayTracing(DefaultParameters) {
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(4.0f, 10.0f, -4.0f);
	Camera cc(camDesc);
	cc.Setup(MainDev);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController);

	gLinearSampler.Setup(MainDev);
	gPointSampler.Setup(MainDev);

	// raster state
	RasterState noBackCull(D3D11_CULL_NONE);
	noBackCull.Setup(MainDev);

	// model
	Model basicModel;
	basicModel.Setup(MainDev);

	// Mesh
	std::vector<std::shared_ptr<Mesh>> meshList;
	meshList = gMF.Load("./UnknownRoom/UnknownRoom.obj");
	for (auto ele : meshList) ele->Setup(MainDev);

	std::shared_ptr<Mesh> plane;
	gMF.Load(BMT_PLANE, plane, 2, 2);
	plane->Setup(MainDev);

	// Common Texture
	CommonTexture BC(L"./UnknownRoom/BC.png");
	BC.Setup(MainDev);

	// DepthTexture
	DepthTexture deTex(WIDTH, HEIGHT);
	deTex.Setup(MainDev);

	// Canvas
	// 摄像机空间下的发线
	Canvas SSNor(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	SSNor.Setup(MainDev);
	// 摄像机空间下的位置
	Canvas SSPos(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	SSPos.Setup(MainDev);

	// 链表头节点
	Canvas fragmentHead(WIDTH, HEIGHT, DXGI_FORMAT_R32_UINT);
	fragmentHead.SetUARes();
	fragmentHead.Setup(MainDev);
	Canvas fragmentColor(WIDTH, HEIGHT * 4, DXGI_FORMAT_R8G8B8A8_UNORM);
	fragmentColor.SetUARes();
	fragmentColor.Setup(MainDev);
	Canvas fragmentDepthAndNext(WIDTH, HEIGHT * 4, DXGI_FORMAT_R32G32_UINT);
	fragmentDepthAndNext.SetUARes();
	fragmentDepthAndNext.Setup(MainDev);
	Canvas fragmentNor(WIDTH, HEIGHT * 4, DXGI_FORMAT_R32G32B32A32_FLOAT);
	fragmentNor.SetUARes();
	fragmentNor.Setup(MainDev);
	Canvas fragmentPos(WIDTH, HEIGHT * 4, DXGI_FORMAT_R32G32B32A32_FLOAT);
	fragmentPos.SetUARes();
	fragmentPos.Setup(MainDev);

	Canvas tracingResult(WIDTH, HEIGHT);
	tracingResult.SetUARes();
	tracingResult.Setup(MainDev);

	Canvas cpyLevel0(WIDTH, HEIGHT);
	cpyLevel0.SetUARes();
	cpyLevel0.Setup(MainDev);

	Canvas Level1(WIDTH / 2, HEIGHT / 2);
	Level1.SetUARes();
	Level1.Setup(MainDev);

	Canvas cpyLevel1(WIDTH / 2, HEIGHT / 2);
	cpyLevel1.SetUARes();
	cpyLevel1.Setup(MainDev);

	Canvas Level2(WIDTH / 4, HEIGHT / 4);
	Level2.SetUARes();
	Level2.Setup(MainDev);

	Canvas cpyLevel2(WIDTH / 4, HEIGHT / 4);
	cpyLevel2.SetUARes();
	cpyLevel2.Setup(MainDev);

	Canvas Level3(WIDTH / 8, HEIGHT / 8);
	Level3.SetUARes();
	Level3.Setup(MainDev);

	Canvas cpyLevel3(WIDTH / 8, HEIGHT / 8);
	cpyLevel3.SetUARes();
	cpyLevel3.Setup(MainDev);

	Canvas Level4(WIDTH / 16, HEIGHT / 16);
	Level4.SetUARes();
	Level4.Setup(MainDev);

	Canvas cpyLevel4(WIDTH / 16, HEIGHT / 16);
	cpyLevel4.SetUARes();
	cpyLevel4.Setup(MainDev);

	// 充当计数器的结构化缓冲区
	StructuredBuffer<int, 1> structureCounter(false);
	structureCounter.Setup(MainDev);

	struct LinkedListData {
		DirectX::XMFLOAT4 data;
	};
	ConstantBuffer<LinkedListData> linkedListData;
	linkedListData.Setup(MainDev);
	linkedListData.GetData().data = { WIDTH, HEIGHT * 6,  WIDTH * HEIGHT * 6, HEIGHT };

	struct MultiLevelData {
		DirectX::XMFLOAT4 data;
	};
	ConstantBuffer<MultiLevelData> multiLevelData;
	multiLevelData.Setup(MainDev);
	multiLevelData.GetData().data = { 0, WIDTH, HEIGHT, HEIGHT };

	// 存储分层结果
	// 存储分层的颜色值
	Canvas fragmentDiffuse(WIDTH, HEIGHT * 6, DXGI_FORMAT_R8G8B8A8_UNORM);
	fragmentDiffuse.SetUARes(true, false);
	fragmentDiffuse.Setup(MainDev);

	// 存储分层的深度值(min, max)
	Canvas fragmentDepth1(WIDTH, HEIGHT * 6, DXGI_FORMAT_R32G32_FLOAT);
	fragmentDepth1.SetUARes();
	fragmentDepth1.Setup(MainDev);

	Canvas fragmentDepth2(WIDTH / 2, (HEIGHT / 2) * 6, DXGI_FORMAT_R32G32_FLOAT);
	fragmentDepth2.SetUARes();
	fragmentDepth2.Setup(MainDev);

	Canvas fragmentDepth3(WIDTH / 4, (HEIGHT / 4) * 6, DXGI_FORMAT_R32G32_FLOAT);
	fragmentDepth3.SetUARes();
	fragmentDepth3.Setup(MainDev);

	Canvas fragmentDepth4(WIDTH / 8, (HEIGHT / 8) * 6, DXGI_FORMAT_R32G32_FLOAT);
	fragmentDepth4.SetUARes();
	fragmentDepth4.Setup(MainDev);

	Canvas fragmentDepth5(WIDTH / 16, (HEIGHT / 16) * 6, DXGI_FORMAT_R32G32_FLOAT);
	fragmentDepth5.SetUARes();
	fragmentDepth5.Setup(MainDev);

	Canvas fragmentDepth6(WIDTH / 32, (HEIGHT / 32) * 6, DXGI_FORMAT_R32G32_FLOAT);
	fragmentDepth6.SetUARes();
	fragmentDepth6.Setup(MainDev);

	Canvas fragmentDepth7(WIDTH / 64, (HEIGHT / 64) * 6, DXGI_FORMAT_R32G32_FLOAT);
	fragmentDepth7.SetUARes();
	fragmentDepth7.Setup(MainDev);

	// Shader
	// 构建屏幕空间基本信息
	VertexShader BasicVS("../Debug/ScreenSpaceBasicVS.cso");
	BasicVS.Setup(MainDev);
	PixelShader BasicPS("../Debug/ScreenSpaceBasicPS.cso");
	BasicPS.Setup(MainDev);
	// 构建屏幕空间信息的两个Shader
	VertexShader BuildLLVS("../Debug/BuildLinkedListVS.cso");
	BuildLLVS.Setup(MainDev);
	PixelShader BuildLLPS("../Debug/BuildLinkedListPS.cso");
	BuildLLPS.Setup(MainDev);
	//
	VertexShader ShowVS("../Debug/ScreenSpaceShowVS.cso");
	ShowVS.Setup(MainDev);
	PixelShader ShowPS("../Debug/ScreenSpaceShowPS.cso");
	ShowPS.Setup(MainDev);

	// 对构建的屏幕信息进行整理
	ComputeShader LLCS("../Debug/LinkedListArrangeCS.cso");
	LLCS.Setup(MainDev);

	// 构建多个level
	ComputeShader FilterCS("../Debug/MultiLevelCS.cso");
	FilterCS.Setup(MainDev);

	// 光线追踪结果
	ComputeShader TracingCS("../Debug/ScreenSpaceReflectionCS.cso");
	TracingCS.Setup(MainDev);

	// pull push
	ComputeShader PullCS("../Debug/PullPhaseCS.cso");
	PullCS.Setup(MainDev);

	ComputeShader PushCS("../Debug/PushPhaseCS.cso");
	PushCS.Setup(MainDev);

	// 显示结果
	VertexShader QuadVS("../Debug/ScreenSpaceReflectionQuadVS.cso");
	QuadVS.Setup(MainDev);
	PixelShader QuadPS("../Debug/ScreenSpaceReflectionQuadPS.cso");
	QuadPS.Setup(MainDev);

	ShadingPass passBasic(&BasicVS, &BasicPS);
	passBasic
		.BindSource(meshList[0].get())
		.BindSource(&SSNor, true, false)
		.BindSource(&SSPos, true, false)
		.BindSource(renderer->GetMainDS(), true, false)
		.BindSource(&cc, SBT_PIXEL_SHADER, PS_CAMERA_DATA_SLOT)
		.BindSource(&cc, SBT_VERTEX_SHADER, VS_CAMERA_DATA_SLOT)
		.BindSource(&basicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSource(&BC, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	ShadingPass pass0(&BuildLLVS, &BuildLLPS);
	pass0
		.BindSource(meshList[0].get())
		.BindSource(&noBackCull)
		.BindSource(&cc, SBT_PIXEL_SHADER, PS_CAMERA_DATA_SLOT)
		.BindSource(&cc, SBT_VERTEX_SHADER, VS_CAMERA_DATA_SLOT)
		.BindSource(&basicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSource(&BC, SBT_PIXEL_SHADER, 0)
		.BindSourceUA(&structureCounter, SBT_PIXEL_SHADER, 0)
		.BindSourceUA(&fragmentHead, SBT_PIXEL_SHADER, 1)
		.BindSourceUA(&fragmentColor, SBT_PIXEL_SHADER, 2)
		.BindSourceUA(&fragmentDepthAndNext, SBT_PIXEL_SHADER, 3)
		.BindSourceUA(&fragmentNor, SBT_PIXEL_SHADER, 4)
		.BindSourceUA(&fragmentPos, SBT_PIXEL_SHADER, 5)
		.BindSource(&linkedListData, SBT_PIXEL_SHADER, 1)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	ComputingPass pass1(&LLCS, { 96, 96 ,1 });
	pass1
		.BindSourceTex(&fragmentColor, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&fragmentHead, SBT_COMPUTE_SHADER, 1)
		.BindSourceTex(&fragmentDepthAndNext, SBT_COMPUTE_SHADER, 2)
		.BindSourceTex(&fragmentNor, SBT_COMPUTE_SHADER, 3)
		.BindSourceTex(&fragmentPos, SBT_COMPUTE_SHADER, 4)
		.BindSourceUA(&fragmentDiffuse, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&fragmentDepth1, SBT_COMPUTE_SHADER, 1)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&linkedListData, SBT_COMPUTE_SHADER, 1);

	ComputingPass pass2(&FilterCS, { 48, 48, 1 });
	pass2
		.BindSourceTex(&fragmentDepth1, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&fragmentDepth2, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&multiLevelData, SBT_COMPUTE_SHADER, 1);

	ComputingPass pass3(&FilterCS, { 24, 24, 1 });
	pass3
		.BindSourceTex(&fragmentDepth2, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&fragmentDepth3, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&multiLevelData, SBT_COMPUTE_SHADER, 1);

	ComputingPass pass4(&FilterCS, { 12, 12, 1 });
	pass4
		.BindSourceTex(&fragmentDepth3, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&fragmentDepth4, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&multiLevelData, SBT_COMPUTE_SHADER, 1);

	ComputingPass pass5(&FilterCS, { 6, 6, 1 });
	pass5
		.BindSourceTex(&fragmentDepth4, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&fragmentDepth5, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&multiLevelData, SBT_COMPUTE_SHADER, 1);

	ComputingPass pass6(&FilterCS, { 3, 3, 1 });
	pass6
		.BindSourceTex(&fragmentDepth5, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&fragmentDepth6, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&multiLevelData, SBT_COMPUTE_SHADER, 1);

	ComputingPass pass7(&FilterCS, { 2, 2, 1 });
	pass7
		.BindSourceTex(&fragmentDepth6, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&fragmentDepth7, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&multiLevelData, SBT_COMPUTE_SHADER, 1);

	ComputingPass passTracing(&TracingCS, { 96, 96, 1 });
	passTracing
		.BindSourceTex(&SSNor, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&SSPos, SBT_COMPUTE_SHADER, 1)
		.BindSourceTex(&fragmentDiffuse, SBT_COMPUTE_SHADER, 2)
		.BindSourceTex(&fragmentDepth1, SBT_COMPUTE_SHADER, 3)
		.BindSourceTex(&fragmentDepth2, SBT_COMPUTE_SHADER, 4)
		.BindSourceTex(&fragmentDepth3, SBT_COMPUTE_SHADER, 5)
		.BindSourceTex(&fragmentDepth4, SBT_COMPUTE_SHADER, 6)
		.BindSourceTex(&fragmentDepth5, SBT_COMPUTE_SHADER, 7)
		.BindSourceTex(&fragmentDepth6, SBT_COMPUTE_SHADER, 8)
		.BindSourceTex(&fragmentDepth7, SBT_COMPUTE_SHADER, 9)
		.BindSourceUA(&tracingResult, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0);

	ComputingPass pullPass0(&PullCS, { 48, 48, 1 });
	pullPass0
		.BindSourceTex(&tracingResult, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level1, SBT_COMPUTE_SHADER, 0);

	ComputingPass pullPass1(&PullCS, { 24, 24, 1 });
	pullPass1
		.BindSourceTex(&Level1, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level2, SBT_COMPUTE_SHADER, 0);

	ComputingPass pullPass2(&PullCS, { 12, 12, 1 });
	pullPass2
		.BindSourceTex(&Level2, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level3, SBT_COMPUTE_SHADER, 0);

	ComputingPass pullPass3(&PullCS, { 6, 6, 1 });
	pullPass3
		.BindSourceTex(&Level3, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level4, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass0(&PushCS, { 12, 12, 1 });
	pushPass0
		.BindSourceTex(&Level4, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&Level3, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel3, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass1(&PushCS, { 24, 24, 1 });
	pushPass1
		.BindSourceTex(&cpyLevel3, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&Level2, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel2, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass2(&PushCS, { 48, 48, 1 });
	pushPass2
		.BindSourceTex(&cpyLevel2, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&Level1, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel1, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass3(&PushCS, { 96, 96, 1 });
	pushPass3
		.BindSourceTex(&cpyLevel1, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&tracingResult, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel0, SBT_COMPUTE_SHADER, 0);

	ShadingPass passShow2(&ShowVS, &ShowPS);
	passShow2
		.BindSource(meshList[0].get())
		.BindSource(renderer->GetMainRT(), true, false)
		.BindSource(renderer->GetMainDS(), true, false)
		.BindSourceTex(&cpyLevel0, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0)
		.BindSource(&basicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSource(&cc, SBT_VERTEX_SHADER, VS_CAMERA_DATA_SLOT);

	ShadingPass passShow(&QuadVS, &QuadPS);
	passShow
		.BindSource(plane.get())
		.BindSource(renderer->GetMainRT(), true, false)
		.BindSource(&basicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSourceTex(&cpyLevel0, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);
	const UINT MaxStorage = WIDTH * HEIGHT * 6;
	mc->Run([&] {
		passBasic.Run(MainDevCtx).End(MainDevCtx);

		renderer->ClearRenderTarget(&fragmentNor);
		renderer->ClearRenderTarget(&fragmentPos);
		renderer->ClearRenderTarget(&fragmentColor);
		renderer->ClearUAV_UINT(&fragmentHead, {MaxStorage, MaxStorage, MaxStorage, MaxStorage});
		//renderer->ClearRenderTarget(&fragmentHead);
		renderer->ClearUAV_UINT(&fragmentDepthAndNext, { MaxStorage, MaxStorage, MaxStorage, MaxStorage });
		//renderer->ClearRenderTarget(&fragmentDepthAndNext);
		renderer->ClearRenderTarget(&fragmentDiffuse);
		renderer->ClearRenderTarget(&fragmentDepth1);
		renderer->ClearRenderTarget(&fragmentDepth2);
		renderer->ClearRenderTarget(&fragmentDepth3);
		renderer->ClearRenderTarget(&fragmentDepth4);
		renderer->ClearRenderTarget(&fragmentDepth5);
		renderer->ClearRenderTarget(&fragmentDepth6);
		renderer->ClearRenderTarget(&fragmentDepth7);
		renderer->ClearRenderTarget(&tracingResult);

		renderer->ClearRenderTarget(&Level1);
		renderer->ClearRenderTarget(&Level2);
		renderer->ClearRenderTarget(&Level3);
		renderer->ClearRenderTarget(&Level4);

		renderer->ClearRenderTarget(&cpyLevel0);
		renderer->ClearRenderTarget(&cpyLevel1);
		renderer->ClearRenderTarget(&cpyLevel2);
		renderer->ClearRenderTarget(&cpyLevel3);

		pass0.Run(MainDevCtx).End(MainDevCtx);
		pass1.Run(MainDevCtx).End(MainDevCtx);

		multiLevelData.GetData().data = { 1, WIDTH / 2, HEIGHT / 2, HEIGHT };
		pass2.Run(MainDevCtx).End(MainDevCtx);

		multiLevelData.GetData().data = { 2, WIDTH / 4, HEIGHT / 4, HEIGHT / 2 };
		pass3.Run(MainDevCtx).End(MainDevCtx);

		multiLevelData.GetData().data = { 3, WIDTH / 8, HEIGHT / 8, HEIGHT / 4 };
		pass4.Run(MainDevCtx).End(MainDevCtx);

		multiLevelData.GetData().data = { 4, WIDTH / 16, HEIGHT / 16, HEIGHT / 8 };
		pass5.Run(MainDevCtx).End(MainDevCtx);

		multiLevelData.GetData().data = { 5, WIDTH / 32, HEIGHT / 32, HEIGHT / 16 };
		pass6.Run(MainDevCtx).End(MainDevCtx);

		multiLevelData.GetData().data = { 6, WIDTH / 64, HEIGHT / 64, HEIGHT / 32 };
		pass7.Run(MainDevCtx).End(MainDevCtx);

		passTracing.Run(MainDevCtx).End(MainDevCtx);

		pullPass0.Run(MainDevCtx).End(MainDevCtx);
		pullPass1.Run(MainDevCtx).End(MainDevCtx);
		pullPass2.Run(MainDevCtx).End(MainDevCtx);
		pullPass3.Run(MainDevCtx).End(MainDevCtx);

		pushPass0.Run(MainDevCtx).End(MainDevCtx);
		pushPass1.Run(MainDevCtx).End(MainDevCtx);
		pushPass2.Run(MainDevCtx).End(MainDevCtx);
		pushPass3.Run(MainDevCtx).End(MainDevCtx);

		passShow2.Run(MainDevCtx).End(MainDevCtx);

		renderer->EndRender();
	});
}

void PullPush(DefaultParameters) {
	gPointSampler.Setup(MainDev);
	// basic shader
	VertexShader BasicVS("../Debug/ScreenSpaceReflectionQuadVS.cso");
	BasicVS.Setup(MainDev);
	PixelShader BasicPS("../Debug/ScreenSpaceReflectionQuadPS.cso");
	BasicPS.Setup(MainDev);
	// Shader
	ComputeShader PullCS("../Debug/PullPhaseCS.cso");
	PullCS.Setup(MainDev);
	ComputeShader PushCS("../Debug/PushPhaseCS.cso");
	PushCS.Setup(MainDev);

	// Based Level
	CommonTexture RefImg(L"./UnknownRoom/TestImage/test2.bmp");
	RefImg.Setup(MainDev);

	// Canvas
	Canvas Level0(WIDTH, HEIGHT);
	Level0.SetUARes();
	Level0.Setup(MainDev);

	Canvas cpyLevel0(WIDTH, HEIGHT);
	cpyLevel0.SetUARes();
	cpyLevel0.Setup(MainDev);

	Canvas Level1(WIDTH / 2, HEIGHT / 2);
	Level1.SetUARes();
	Level1.Setup(MainDev);

	Canvas cpyLevel1(WIDTH / 2, HEIGHT / 2);
	cpyLevel1.SetUARes();
	cpyLevel1.Setup(MainDev);

	Canvas Level2(WIDTH / 4, HEIGHT / 4);
	Level2.SetUARes();
	Level2.Setup(MainDev);

	Canvas cpyLevel2(WIDTH / 4, HEIGHT / 4);
	cpyLevel2.SetUARes();
	cpyLevel2.Setup(MainDev);

	Canvas Level3(WIDTH / 8, HEIGHT / 8);
	Level3.SetUARes();
	Level3.Setup(MainDev);

	Canvas cpyLevel3(WIDTH / 8, HEIGHT / 8);
	cpyLevel3.SetUARes();
	cpyLevel3.Setup(MainDev);

	Canvas Level4(WIDTH / 16, HEIGHT / 16);
	Level4.SetUARes();
	Level4.Setup(MainDev);

	// Mesh
	std::shared_ptr<Mesh> plane;
	gMF.Load(BMT_PLANE, plane, 2, 2);
	plane->Setup(MainDev);

	// pass
	ShadingPass loadPass(&BasicVS, &BasicPS);
	loadPass
		.BindSource(plane.get())
		.BindSource(&Level0, true, false)
		.BindSource(&RefImg, SBT_PIXEL_SHADER, 0)
		.BindSource(&gPointSampler, SBT_PIXEL_SHADER, 0);

	ComputingPass pullPass0(&PullCS, {48, 48, 1});
	pullPass0
		.BindSourceTex(&Level0, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level1, SBT_COMPUTE_SHADER, 0);

	ComputingPass pullPass1(&PullCS, { 24, 24, 1 });
	pullPass1
		.BindSourceTex(&Level1, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level2, SBT_COMPUTE_SHADER, 0);

	ComputingPass pullPass2(&PullCS, { 12, 12, 1 });
	pullPass2
		.BindSourceTex(&Level2, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level3, SBT_COMPUTE_SHADER, 0);

	ComputingPass pullPass3(&PullCS, { 6, 6, 1 });
	pullPass3
		.BindSourceTex(&Level3, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&Level4, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass0(&PushCS, { 12, 12, 1 });
	pushPass0
		.BindSourceTex(&Level4, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&Level3, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel3, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass1(&PushCS, { 24, 24, 1 });
	pushPass1
		.BindSourceTex(&cpyLevel3, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&Level2, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel2, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass2(&PushCS, { 48, 48, 1 });
	pushPass2
		.BindSourceTex(&cpyLevel2, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&Level1, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel1, SBT_COMPUTE_SHADER, 0);

	ComputingPass pushPass3(&PushCS, { 96, 96, 1 });
	pushPass3
		.BindSourceTex(&cpyLevel1, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&Level0, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&cpyLevel0, SBT_COMPUTE_SHADER, 0);

	ShadingPass showPass(&BasicVS, &BasicPS);
	showPass
		.BindSource(plane.get())
		.BindSource(renderer->GetMainRT(), true, false)
		.BindSourceTex(&cpyLevel0, SBT_PIXEL_SHADER, 0)
		.BindSource(&gPointSampler, SBT_PIXEL_SHADER, 0);

	mc->Run([&] {
		renderer->ClearRenderTarget(&Level0);
		renderer->ClearRenderTarget(&Level1);
		renderer->ClearRenderTarget(&Level2);
		renderer->ClearRenderTarget(&Level3);
		renderer->ClearRenderTarget(&Level4);

		renderer->ClearRenderTarget(&cpyLevel0);
		renderer->ClearRenderTarget(&cpyLevel1);
		renderer->ClearRenderTarget(&cpyLevel2);
		renderer->ClearRenderTarget(&cpyLevel3);

		loadPass.Run(MainDevCtx).End(MainDevCtx);
		pullPass0.Run(MainDevCtx).End(MainDevCtx);
		pullPass1.Run(MainDevCtx).End(MainDevCtx);
		pullPass2.Run(MainDevCtx).End(MainDevCtx);
		pullPass3.Run(MainDevCtx).End(MainDevCtx);
		pushPass0.Run(MainDevCtx).End(MainDevCtx);
		pushPass1.Run(MainDevCtx).End(MainDevCtx);
		pushPass2.Run(MainDevCtx).End(MainDevCtx);
		pushPass3.Run(MainDevCtx).End(MainDevCtx);
		showPass.Run(MainDevCtx).End(MainDevCtx);
		renderer->EndRender();
	});
}

void MyALG(DefaultParameters) {
	// 创建camera
	CAMERA_DESC camDesc(WIDTH, HEIGHT);
	camDesc.fov = 0.79;
	camDesc.lookAt = DirectX::XMFLOAT3();
	camDesc.position = DirectX::XMFLOAT3(0.0f, 5.0f, -3.0f);
	Camera cc(camDesc);
	cc.Setup(MainDev);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController);
	gLinearSampler.Setup(MainDev);
	gPointSampler.Setup(MainDev);
// 预先需要每个三角形的ID图
// 渲染场景中所有面的空间信息位置信息
	VertexShader UpdatePosVS("../Debug/MyALGUpdatePosVS.cso");
	UpdatePosVS.Setup(MainDev);
	PixelShader UpdatePosPS("../Debug/MyALGUpdatePosPS.cso");
	UpdatePosPS.Setup(MainDev);
// 构建屏幕空间信息G-Buffer
	VertexShader SSDataVS("../Debug/MyALGSSDataVS.cso");
	SSDataVS.Setup(MainDev);
	PixelShader SSDataPS("../Debug/MyALGSSDataPS.cso");
	SSDataPS.Setup(MainDev);
// 处理屏幕下的ID信息
	ComputeShader ClusterArrangeCS2("../Debug/MyALGArrangeClusterCoreCS2.cso");
	ClusterArrangeCS2.Setup(MainDev);
	ComputeShader NormalizeAscriptionCS("../Debug/MyALGNormalizeAscriptionCS.cso");
	NormalizeAscriptionCS.Setup(MainDev);
// 利用管线对反射样本点进行聚类
	VertexShader ClusterRefVS("../Debug/MyALGArrangeClusterCoreVS.cso");
	ClusterRefVS.Setup(MainDev);
	PixelShader ClusterRefPS("../Debug/MyALGArrangeClusterCorePS.cso");
	ClusterRefPS.Setup(MainDev);
// 将管线结果进行平均化
	ComputeShader ClusterAverageCS("../Debug/MyALGAverageClusterCoreCS.cso");
	ClusterAverageCS.Setup(MainDev);
// 从屏幕空间中选择点作为反射参考点
	ComputeShader RefPntCS("../Debug/MyALGRefPoint2CS.cso");
	RefPntCS.Setup(MainDev);
// 构建阴影图——将场景中的所有点映射到反射参考点的空间中
	ComputeShader ConstructShadowMapCS("../Debug/MyALGCstShadowMap2CS.cso");
	ConstructShadowMapCS.Setup(MainDev);
// 利用管线构造阴影图
	VertexShader PPConstructShadowMapVS("../Debug/MyALGCstShadowMapVS.cso");
	PPConstructShadowMapVS.Setup(MainDev);
	GeometryShader PPConstructShadowMapGS("../Debug/MyALGCstShadowMapGS.cso");
	PPConstructShadowMapGS.Setup(MainDev);
	PixelShader PPConstructShadowMapPS("../Debug/MyALGCstShadowMapPS.cso");
	PPConstructShadowMapPS.Setup(MainDev);
// 利用管线构造阴影图2
	VertexShader PPConstructShadowMap2VS("../Debug/MyALGCstShadowMap2VS.cso");
	PPConstructShadowMap2VS.Setup(MainDev);
	GeometryShader PPConstructShadowMap2GS("../Debug/MyALGCstShadowMap2GS.cso");
	PPConstructShadowMap2GS.Setup(MainDev);
	PixelShader PPConstructShadowMap2PS("../Debug/MyALGCstShadowMap2PS.cso");
	PPConstructShadowMap2PS.Setup(MainDev);
// 利用管线构造阴影图(仅包含Diffuse)
	VertexShader PPShowShadowMapVS("../Debug/MyALGShowShadowMap2VS.cso");
	PPShowShadowMapVS.Setup(MainDev);
	GeometryShader PPShowShadowMapGS("../Debug/MyALGShowShadowMap2GS.cso");
	PPShowShadowMapGS.Setup(MainDev);
	PixelShader PPShowShadowMapPS("../Debug/MyALGShowShadowMap2PS.cso");
	PPShowShadowMapPS.Setup(MainDev);
// 展示某个反射采样点的视角所见内容
	VertexShader ShowRefViewVS("../Debug/MyALGShowRefPntViewVS.cso");
	ShowRefViewVS.Setup(MainDev);
	PixelShader ShowRefViewPs("../Debug/MyALGShowRefPntViewPS.cso");
	ShowRefViewPs.Setup(MainDev);
// 计算当前视口每个像素点参考的反射点
	ComputeShader CalculateAscripitionCS("../Debug/MyALGCalAscriptionCS.cso");
	CalculateAscripitionCS.Setup(MainDev);
// 对反射结果进行采样计算
	ComputeShader SampleReflectionCS("../Debug/MyALGSampleReflectionCS.cso");
	SampleReflectionCS.Setup(MainDev);
// 展示场景点化的结果
	ComputeShader ShowPointsCS("../Debug/MyALGShowPointsCS.cso");
	ShowPointsCS.Setup(MainDev);
// 展示否幅图片的边缘检测结果
	ComputeShader ShowEdgeDetect("../Debug/MyALGEdgeDetect.cso");
	ShowEdgeDetect.Setup(MainDev);
// 计算当前像素点应该采样哪个反射位置
	ComputeShader ShowAscriptionCS("../Debug/MyALGShowAscriptionCS.cso");
	ShowAscriptionCS.Setup(MainDev);
// 构建阴影图——将场景中的所有点映射到反射参考点的空间中
	ComputeShader ShowShadowMapCS("../Debug/MyALGShowShadowMapCS.cso");
	ShowShadowMapCS.Setup(MainDev);
// 展示后处理结果
	VertexShader ShowPostProcessVS("../Debug/MyALGShowPostProcessVS.cso");
	ShowPostProcessVS.Setup(MainDev);
	PixelShader ShowPostProcessPS("../Debug/MyALGShowPostProcessPS.cso");
	ShowPostProcessPS.Setup(MainDev);

	std::vector<std::shared_ptr<Mesh>> meshList;
	meshList = gMF.Load("./UnknownRoom/UnknownRoom.obj");
	for (auto& iter : meshList) iter->Setup(MainDev);

	std::shared_ptr<Mesh> plane;
	gMF.Load(BMT_PLANE, plane, 2.0f, 2.0f);
	plane->Setup(MainDev);

	Model BasicModel;
	BasicModel.Setup(MainDev);

	CommonTexture BasicColor(L"./UnknownRoom/BC.png");
	BasicColor.Setup(MainDev);

	CommonTexture IDTex(L"./UnknownRoom/ID.png");
	IDTex.Setup(MainDev);

	const float ScePntNum = 150;
	const int ScePntNumInt = 150;
	// Raster state
	RasterState noCullingRS(D3D11_CULL_NONE);
	noCullingRS.Setup(MainDev);

	D3D11_VIEWPORT GeoViewPort;
	GeoViewPort.Height = ScePntNum;
	GeoViewPort.Width = ScePntNum;
	GeoViewPort.MinDepth = 0.0f;
	GeoViewPort.MaxDepth = 1.0f;
	GeoViewPort.TopLeftX = GeoViewPort.TopLeftY = 0;
// Canvas
// 记录采样的点，包括点的位置，法线，正副切线
	Canvas ScePntPos(ScePntNum, ScePntNum, DXGI_FORMAT_R32G32B32A32_FLOAT);
	ScePntPos.SetUARes();
	ScePntPos.Setup(MainDev);
	Canvas ScePntNor(ScePntNum, ScePntNum, DXGI_FORMAT_R32G32B32A32_FLOAT);
	ScePntNor.SetUARes();
	ScePntNor.Setup(MainDev);
	Canvas ScePntDiffuse(ScePntNum, ScePntNum, DXGI_FORMAT_R8G8B8A8_UNORM);
	ScePntDiffuse.SetUARes();
	ScePntDiffuse.Setup(MainDev);

	ConstantBuffer<DirectX::XMFLOAT4> initPntData;
	initPntData.GetData().x = ScePntNum;
	initPntData.GetData().y = ScePntNum;
	initPntData.Setup(MainDev);

	struct ScePntStruct {
		DirectX::XMFLOAT4 vertPos;
		DirectX::XMFLOAT4 vertNor;
	};

// 记录当前视口下每个像素点的位置以及法线
	Canvas SSWPos(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	SSWPos.Setup(MainDev);
	Canvas SSWNor(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	SSWNor.Setup(MainDev);
	Canvas SSID(WIDTH, HEIGHT, DXGI_FORMAT_R32_UINT);
	SSID.SetUARes();
	SSID.Setup(MainDev);

	const int RefPntNum = 10;

	struct RefEleData
	{
		DirectX::XMFLOAT4 wPos;
		DirectX::XMFLOAT4 wRef;
		DirectX::XMFLOAT4 vRef;
		DirectX::XMFLOAT4 wNor;
		DirectX::XMFLOAT4X4 refMatrix;
		DirectX::XMFLOAT4X4 refProjMatrix;
	};

	StructuredBuffer<RefEleData, RefPntNum * RefPntNum> RefViewMatrixs(false);
	RefViewMatrixs.Setup(MainDev);

	ConstantBuffer<DirectX::XMFLOAT4> refPntData;
	refPntData.GetData().x = WIDTH;
	refPntData.GetData().y = HEIGHT;
	refPntData.GetData().z = RefPntNum;
	refPntData.GetData().w = RefPntNum;
	refPntData.Setup(MainDev);

// 利用渲染管线将反射样本点进行聚类
	D3D11_BLEND_DESC ClusterBlendDesc;
	ZeroMemory(&ClusterBlendDesc, sizeof(ClusterBlendDesc));
	ClusterBlendDesc.AlphaToCoverageEnable = FALSE;
	ClusterBlendDesc.IndependentBlendEnable = TRUE;
	ClusterBlendDesc.RenderTarget[0].BlendEnable = ClusterBlendDesc.RenderTarget[1].BlendEnable = TRUE;
	// src.rgb * (1, 1, 1) + dest.rgb * (1, 1, 1)
	ClusterBlendDesc.RenderTarget[0].SrcBlend = ClusterBlendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ONE;
	ClusterBlendDesc.RenderTarget[0].DestBlend = ClusterBlendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ONE;
	ClusterBlendDesc.RenderTarget[0].BlendOp = ClusterBlendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
	// src.a * 1 + desc.a * 1
	ClusterBlendDesc.RenderTarget[0].SrcBlendAlpha = ClusterBlendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ONE;
	ClusterBlendDesc.RenderTarget[0].DestBlendAlpha = ClusterBlendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ONE;
	ClusterBlendDesc.RenderTarget[0].BlendOpAlpha = ClusterBlendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	ClusterBlendDesc.RenderTarget[0].RenderTargetWriteMask = ClusterBlendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	BlendState ClusterBlendState(ClusterBlendDesc);
	ClusterBlendState.Setup(MainDev);

	Canvas ClusterResultPos(RefPntNum, RefPntNum, DXGI_FORMAT_R32G32B32A32_FLOAT);
	ClusterResultPos.SetUARes();
	ClusterResultPos.Setup(MainDev);
	Canvas ClusterResultNor(RefPntNum, RefPntNum, DXGI_FORMAT_R32G32B32A32_FLOAT);
	ClusterResultNor.SetUARes();
	ClusterResultNor.Setup(MainDev);

	D3D11_VIEWPORT ClusterViewPort;
	ClusterViewPort.Height = RefPntNum;
	ClusterViewPort.Width = RefPntNum;
	ClusterViewPort.MaxDepth = 1.0f;
	ClusterViewPort.MinDepth = 0.0f;
	ClusterViewPort.TopLeftX = ClusterViewPort.TopLeftY = 0;

// 记录当前构建的每个反射位置的阴影图
	const int SubShadowMapSize = 128;
	const int ShadowMapSize = SubShadowMapSize * RefPntNum;
	Canvas ShadowMapDiffuse(ShadowMapSize, ShadowMapSize, DXGI_FORMAT_R8G8B8A8_UNORM);
	ShadowMapDiffuse.SetUARes();
	ShadowMapDiffuse.Setup(MainDev);
	Canvas ShadowMapTexPos(ShadowMapSize, ShadowMapSize, DXGI_FORMAT_R32G32_UINT);
	ShadowMapTexPos.SetUARes();
	ShadowMapTexPos.Setup(MainDev);
	Canvas ShadowMapDepth(ShadowMapSize, ShadowMapSize, DXGI_FORMAT_R32_UINT);
	ShadowMapDepth.SetUARes(true, true);
	ShadowMapDepth.Setup(MainDev);
	struct CstShadowMapData {
		DirectX::XMFLOAT4 exData;
		DirectX::XMFLOAT4 shadowMapSize;
	};

	DepthTexture ShadowMapDepth2(ShadowMapSize, ShadowMapSize);
	ShadowMapDepth2.Setup(MainDev);
	Canvas ShadowMapPos(ShadowMapSize, ShadowMapSize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	ShadowMapPos.Setup(MainDev);
	Canvas ShadowMapNor(ShadowMapSize, ShadowMapSize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	ShadowMapNor.Setup(MainDev);

	ConstantBuffer<CstShadowMapData> cstShadowMapData;
	cstShadowMapData.GetData().exData = DirectX::XMFLOAT4(RefPntNum, RefPntNum, RefPntNum * RefPntNum, 0);
	cstShadowMapData.GetData().shadowMapSize = DirectX::XMFLOAT4(SubShadowMapSize, SubShadowMapSize, ShadowMapSize, ShadowMapSize);
	cstShadowMapData.Setup(MainDev);

	D3D11_VIEWPORT ShadowMapViewPort;
	ShadowMapViewPort.Height = ShadowMapViewPort.Width = ShadowMapSize;
	ShadowMapViewPort.MaxDepth = 1.0f;
	ShadowMapViewPort.MinDepth = 0.0f;
	ShadowMapViewPort.TopLeftX = ShadowMapViewPort.TopLeftY = 0;

	struct ProjMatrixData {
		DirectX::XMFLOAT4 PerProjSettingData;
		DirectX::XMFLOAT4X4 PerProjMatrix;
		DirectX::XMFLOAT4X4 PerProjMatrixInv;
		DirectX::XMFLOAT4 OrtProjSettingData;
		DirectX::XMFLOAT4X4 OrtProjMatrix;
		DirectX::XMFLOAT4X4 OrtProjMatrixInv;
	};

	ConstantBuffer<ProjMatrixData> ProjectMatrixData;
	DirectX::XMFLOAT4X4 SMProjMatrix;
	DirectX::XMFLOAT4X4 SMProjMatrixInv;
	DirectX::XMMATRIX tempProjMatrix;

	DirectX::XMFLOAT4 SMPerProjMatrixSetting = { 0.24f, 1.0f, 0.05f, 50.0f };
	tempProjMatrix = DirectX::XMMatrixPerspectiveFovLH(SMPerProjMatrixSetting.x, SMPerProjMatrixSetting.y,
		SMPerProjMatrixSetting.z, SMPerProjMatrixSetting.w);
	DirectX::XMStoreFloat4x4(&SMProjMatrix, tempProjMatrix);
	DirectX::XMStoreFloat4x4(&SMProjMatrixInv, DirectX::XMMatrixInverse(nullptr, tempProjMatrix));

	ProjectMatrixData.GetData().PerProjMatrix = SMProjMatrix;
	ProjectMatrixData.GetData().PerProjMatrixInv = SMProjMatrixInv;
	ProjectMatrixData.GetData().PerProjSettingData = SMPerProjMatrixSetting;

	DirectX::XMFLOAT4 SMOrtProjMatrixSetting = { 5.0f, 5.0f, 0.05f, 50.0f };
	tempProjMatrix = DirectX::XMMatrixOrthographicLH(SMOrtProjMatrixSetting.x, SMOrtProjMatrixSetting.y,
		SMOrtProjMatrixSetting.z, SMOrtProjMatrixSetting.w);
	DirectX::XMStoreFloat4x4(&SMProjMatrix, tempProjMatrix);
	DirectX::XMStoreFloat4x4(&SMProjMatrixInv, DirectX::XMMatrixInverse(nullptr, tempProjMatrix));

	ProjectMatrixData.GetData().OrtProjMatrix = SMProjMatrix;
	ProjectMatrixData.GetData().OrtProjMatrixInv = SMProjMatrixInv;
	ProjectMatrixData.GetData().OrtProjSettingData = SMOrtProjMatrixSetting;

	ProjectMatrixData.Setup(MainDev);

	D3D11_VIEWPORT ShadowMapViewPortssss[RefPntNum * RefPntNum];
	
	for (DirectX::XMUINT2 iter = { 0, 0 }; iter.y < RefPntNum; ++iter.y) {
		for (iter.x = 0; iter.x < RefPntNum; ++iter.x) {
			UINT index = iter.x + iter.y * RefPntNum;
			ShadowMapViewPortssss[index].Height = ShadowMapViewPortssss[index].Width = SubShadowMapSize;
			ShadowMapViewPortssss[index].MaxDepth = 1.0f;
			ShadowMapViewPortssss[index].MinDepth = 0.0f;
			ShadowMapViewPortssss[index].TopLeftX = iter.x * SubShadowMapSize;
			ShadowMapViewPortssss[index].TopLeftY = iter.y * SubShadowMapSize;
		}
	}
	D3D11_VIEWPORT* ShadowMapViewPortPointer = nullptr;
	ConstantBuffer<DirectX::XMUINT4> InstanceData;
	InstanceData.GetData().x = 0;
	InstanceData.Setup(MainDev);

// 记录当前场景点化的结果
	Canvas DepthTexture(WIDTH, HEIGHT, DXGI_FORMAT_R32_FLOAT);
	DepthTexture.SetUARes(true, true);
	DepthTexture.Setup(MainDev);

	Canvas PointsResult(WIDTH, HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);
	PointsResult.SetUARes();
	PointsResult.Setup(MainDev);

// 记录当前视点下每个像素应该采样哪个反射点
	Canvas AscriptionMap(WIDTH, HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);
	AscriptionMap.SetUARes();
	AscriptionMap.Setup(MainDev);
	Canvas AscriptionData(WIDTH, HEIGHT, DXGI_FORMAT_R32_UINT);
	AscriptionData.SetUARes();
	AscriptionData.Setup(MainDev);

// 记录当前视点下每个像素的反射结果
	Canvas ReflectionResultPos(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	ReflectionResultPos.SetUARes();
	ReflectionResultPos.Setup(MainDev);

// 记录某幅图的描边结果
	Canvas EdgeDetectResult(WIDTH, HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);
	EdgeDetectResult.SetUARes();
	EdgeDetectResult.Setup(MainDev);

// 当前屏幕的分块(行/列)个数
	const UINT ScreenTiles = 5; // 屏幕被分割成行列个K个，总共K x K个区域
	const UINT TileMaxSubTile = 36; // 每个区域最大的面数量
	Canvas TileIDIndex(ScreenTiles * ScreenTiles * TileMaxSubTile, 1, DXGI_FORMAT_R32_UINT);
	TileIDIndex.SetUARes(true, true);
	TileIDIndex.Setup(MainDev);

	// 充当计数器的结构化缓冲区
	StructuredBuffer<int, 1> structureCounter(false);
	structureCounter.Setup(MainDev);

// pass
	ShadingPass updatePosPass(&UpdatePosVS, &UpdatePosPS);
	updatePosPass
		.BindSource(&noCullingRS, &GeoViewPort)
		.BindSource(meshList[0].get())
		.BindSource(&ScePntPos, true, false)
		.BindSource(&ScePntNor, true, false)
		.BindSource(&ScePntDiffuse, true, false)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0)
		.BindSourceTex(&BasicColor, SBT_PIXEL_SHADER, 0)
		.BindSource(&BasicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSource(&cc, SBT_VERTEX_SHADER, VS_CAMERA_DATA_SLOT);

	ShadingPass ssDataPass(&SSDataVS, &SSDataPS);
	ssDataPass
		.BindSource(&SSWPos, true, false)
		.BindSource(&SSWNor, true, false)
		.BindSource(&SSID, false, false)
		.BindSource(renderer->GetMainDS(), true, false)
		.BindSource(meshList[0].get())
		.BindSource(&BasicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSource(&cc, SBT_VERTEX_SHADER, VS_CAMERA_DATA_SLOT)
		.BindSourceTex(&IDTex, SBT_PIXEL_SHADER, 0)
		.BindSource(&gPointSampler, SBT_PIXEL_SHADER, 0);

	ComputingPass refPntPass(&RefPntCS, { 1, 1, 1 });
	refPntPass
		.BindSourceTex(&SSWPos, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&SSWNor, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&RefViewMatrixs, SBT_COMPUTE_SHADER, 0)
		.BindSource(&refPntData, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 1);

	ShadingPass PPClusterRefPass(&ClusterRefVS, &ClusterRefPS);
	PPClusterRefPass
		.SpecialDrawCall(WIDTH * HEIGHT)
		.BindSource(&ClusterBlendState)
		.BindSource(nullptr, &ClusterViewPort)
		.BindSource(&ClusterResultPos, true, false)
		.BindSource(&ClusterResultNor, true, false)
		.BindSourceTex(&SSWPos, SBT_VERTEX_SHADER, 0)
		.BindSourceTex(&SSWNor, SBT_VERTEX_SHADER, 1)
		.BindSourceTex(&SSID, SBT_VERTEX_SHADER, 2)
		.BindSourceTex(&TileIDIndex, SBT_VERTEX_SHADER, 3)
		.BindSource(&refPntData, SBT_VERTEX_SHADER, 0);

	ComputingPass ClusterAveragePass(&ClusterAverageCS, { RefPntNum / 10, RefPntNum / 10, 1 });
	ClusterAveragePass
		.BindSourceTex(&ClusterResultPos, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&ClusterResultNor, SBT_COMPUTE_SHADER, 1)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&refPntData, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&RefViewMatrixs, SBT_COMPUTE_SHADER, 0);

	ComputingPass cstShadowMapPass(&ConstructShadowMapCS, { ScePntNum / 10, ScePntNum / 10, 1 });
	cstShadowMapPass
		.BindSourceBuf(&RefViewMatrixs, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&ScePntPos, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&ShadowMapTexPos, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&ShadowMapDepth, SBT_COMPUTE_SHADER, 1)
		.BindSource(&cstShadowMapData, SBT_COMPUTE_SHADER, 0);

	ShadingPass PPCstShadowMapPass(&PPConstructShadowMapVS, &PPConstructShadowMapPS, &PPConstructShadowMapGS);
	PPCstShadowMapPass
		.BindSource(nullptr, &ShadowMapViewPort)
		.SpecialDrawCall(ScePntNumInt * ScePntNumInt)
		.BindSource(&ShadowMapDepth2, true, false)
		.BindSource(&ShadowMapPos, true, false)
		.BindSource(&ShadowMapNor, true, false)
		.BindSource(&ShadowMapDiffuse, true, false)
		.BindSourceTex(&ScePntPos, SBT_VERTEX_SHADER, 0)
		.BindSourceTex(&ScePntNor, SBT_VERTEX_SHADER, 1)
		.BindSourceTex(&ScePntDiffuse, SBT_VERTEX_SHADER, 2)
		.BindSource(&initPntData, SBT_VERTEX_SHADER, 0)
		.BindSourceBuf(&RefViewMatrixs, SBT_GEOMETRY_SHADER, 0)
		.BindSource(&cstShadowMapData, SBT_GEOMETRY_SHADER, 0);

	ShadingPass PPCstShadowMapPass2(&PPConstructShadowMap2VS, &PPConstructShadowMap2PS, &PPConstructShadowMap2GS);
	PPCstShadowMapPass2
		.BindSource(meshList[0].get())
		.BindSource(&ShadowMapPos, false, false)
		.BindSource(&ShadowMapNor, false, false)
		.BindSource(&ShadowMapDepth2, false, false)
		.BindSource(&BasicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSourceBuf(&RefViewMatrixs, SBT_GEOMETRY_SHADER, 0)
		.BindSource(&ProjectMatrixData, SBT_GEOMETRY_SHADER, 0)
		.BindSource(&refPntData, SBT_GEOMETRY_SHADER, 1)
		.BindSource(&cc, SBT_GEOMETRY_SHADER, 3);

	ShadingPass PPShowShadowMap(&PPShowShadowMapVS, &PPShowShadowMapPS, &PPShowShadowMapGS);
	PPShowShadowMap
		.BindSource(meshList[0].get())
		.BindSource(&ShadowMapDiffuse, false, false)
		.BindSource(&ShadowMapDepth2, false, false)
		.BindSource(&BasicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSourceBuf(&RefViewMatrixs, SBT_GEOMETRY_SHADER, 0)
		.BindSource(&ProjectMatrixData, SBT_GEOMETRY_SHADER, 0)
		.BindSource(&refPntData, SBT_GEOMETRY_SHADER, 1)
		.BindSource(&cc, SBT_GEOMETRY_SHADER, 3)
		.BindSource(&BasicColor, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	ComputingPass calculateAscriptionPass(&CalculateAscripitionCS, { WIDTH / 10, HEIGHT / 10, 1 });
	calculateAscriptionPass
		.BindSourceBuf(&RefViewMatrixs, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&SSWPos, SBT_COMPUTE_SHADER, 1)
		.BindSourceTex(&SSWNor, SBT_COMPUTE_SHADER, 2)
		.BindSourceTex(&SSID, SBT_COMPUTE_SHADER, 3)
		.BindSourceTex(&TileIDIndex, SBT_COMPUTE_SHADER, 4)
		.BindSourceUA(&AscriptionData, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&refPntData, SBT_COMPUTE_SHADER, 1);

	ComputingPass reflectionResultPass(&SampleReflectionCS, { WIDTH / 10, HEIGHT / 10, 1 });
	reflectionResultPass
		.BindSourceTex(&AscriptionData, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&ShadowMapDiffuse, SBT_COMPUTE_SHADER, 1)
		.BindSourceTex(&SSWPos, SBT_COMPUTE_SHADER, 2)
		.BindSourceTex(&SSWNor, SBT_COMPUTE_SHADER, 3)
		.BindSourceTex(&ScePntPos, SBT_COMPUTE_SHADER, 4)
		.BindSourceTex(&ScePntNor, SBT_COMPUTE_SHADER, 5)
		.BindSourceBuf(&RefViewMatrixs, SBT_COMPUTE_SHADER, 6)
		.BindSourceTex(&ShadowMapPos, SBT_COMPUTE_SHADER, 7)
		.BindSourceTex(&ShadowMapNor, SBT_COMPUTE_SHADER, 8)
		.BindSourceTex(&ShadowMapDepth2, SBT_COMPUTE_SHADER, 9)
		.BindSourceUA(&ReflectionResultPos, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cstShadowMapData, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 1)
		.BindSource(&ProjectMatrixData, SBT_COMPUTE_SHADER, 2);

	ComputingPass clusterCoreArrangePass(&ClusterArrangeCS2, { WIDTH / 10, HEIGHT / 10, 1 });
	clusterCoreArrangePass
		.BindSourceTex(&SSID, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&TileIDIndex, SBT_COMPUTE_SHADER, 0);

	ComputingPass normalizeAscriptionPass(&NormalizeAscriptionCS, { ScreenTiles * ScreenTiles * TileMaxSubTile / 10, 1, 1 });
	normalizeAscriptionPass
		.BindSourceUA(&TileIDIndex, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&structureCounter, SBT_COMPUTE_SHADER, 1);

	ShadingPass showRefPntViewPass(&ShowRefViewVS, &ShowRefViewPs);
	showRefPntViewPass
		.BindSource(meshList[0].get())
		.BindSource(renderer->GetMainDS(), true, false)
		.BindSource(renderer->GetMainRT(), true ,false)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0)
		.BindSourceBuf(&RefViewMatrixs, SBT_VERTEX_SHADER, 0)
		.BindSource(&ProjectMatrixData, SBT_VERTEX_SHADER, 0)
		.BindSource(&BasicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSource(&cc, SBT_VERTEX_SHADER, 2)
		.BindSourceTex(&BasicColor, SBT_PIXEL_SHADER, 0);

	ComputingPass showShadowMapPass(&ShowShadowMapCS, { ScePntNum / 10, ScePntNum / 10, 1 });
	showShadowMapPass
		.BindSourceBuf(&RefViewMatrixs, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&ScePntPos, SBT_COMPUTE_SHADER, 1)
		.BindSourceTex(&BasicColor, SBT_COMPUTE_SHADER, 2)
		.BindSourceUA(&ShadowMapDiffuse, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&ShadowMapDepth, SBT_COMPUTE_SHADER, 1)
		.BindSource(&cstShadowMapData, SBT_COMPUTE_SHADER, 0)
		.BindSource(&initPntData, SBT_COMPUTE_SHADER, 1);

	ComputingPass showEdgeDetectPass(&ShowEdgeDetect, { WIDTH / 10, HEIGHT / 10, 1 });
	showEdgeDetectPass
		.BindSourceTex(&AscriptionMap, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&EdgeDetectResult, SBT_COMPUTE_SHADER, 0);

	ComputingPass showPointsPass(&ShowPointsCS, { ScePntNum / 10, ScePntNum / 10, 1 });
	showPointsPass
		.BindSourceTex(&ScePntPos, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&BasicColor, SBT_COMPUTE_SHADER, 1)
		.BindSourceUA(&DepthTexture, SBT_COMPUTE_SHADER, 0)
		.BindSourceUA(&PointsResult, SBT_COMPUTE_SHADER, 1)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&initPntData, SBT_COMPUTE_SHADER, 1);

	ComputingPass showAscriptionPass(&ShowAscriptionCS, { WIDTH / 10, HEIGHT / 10, 1 });
	showAscriptionPass
		.BindSourceBuf(&RefViewMatrixs, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&SSWPos, SBT_COMPUTE_SHADER, 1)
		.BindSourceTex(&SSWNor, SBT_COMPUTE_SHADER, 2)
		.BindSourceTex(&SSID, SBT_COMPUTE_SHADER, 3)
		.BindSourceTex(&TileIDIndex, SBT_COMPUTE_SHADER, 4)
		.BindSourceUA(&AscriptionMap, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0)
		.BindSource(&refPntData, SBT_COMPUTE_SHADER, 1);

	// 展示后处理结果
	ShadingPass showPostProcessPass(&ShowPostProcessVS, &ShowPostProcessPS);
	showPostProcessPass
		.BindSource(plane.get())
		.BindSource(renderer->GetMainRT(), true, false)
		.BindSourceTex(&ReflectionResultPos, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	mc->Run([&] {
		// 初始化场景中的点
		//updatePosPass.Run(MainDevCtx).End(MainDevCtx);
		// 渲染当前视口信息G-buffer
		renderer->ClearUAV_UINT(&SSID, { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX });
		ssDataPass.Run(MainDevCtx).End(MainDevCtx);

		// 计算分块情况
		renderer->ClearUAV_UINT(&TileIDIndex);
		clusterCoreArrangePass.Run(MainDevCtx).End(MainDevCtx);
		normalizeAscriptionPass.Run(MainDevCtx).End(MainDevCtx);
		// 计算当前反射样本点的聚类
		PPClusterRefPass.Run(MainDevCtx).End(MainDevCtx);
		// 对当前反射样本点聚类进行平均，求出聚类核心信息
		ClusterAveragePass.Run(MainDevCtx).End(MainDevCtx);
		// 计算当前反射采样点
		//refPntPass.Run(MainDevCtx).End(MainDevCtx);
		// 计算当前视口下每个采样点的归属
		renderer->ClearRenderTarget(&AscriptionMap);
		renderer->ClearRenderTarget(&AscriptionData);
		showAscriptionPass.Run(MainDevCtx).End(MainDevCtx);
		showEdgeDetectPass.Run(MainDevCtx).End(MainDevCtx);

		calculateAscriptionPass.Run(MainDevCtx).End(MainDevCtx);

		// 构造反射图
		//renderer->ClearUAV_UINT(&ShadowMapDepth, { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX });
		//renderer->ClearRenderTarget(&ShadowMapDiffuse);
		//renderer->ClearRenderTarget(&ShadowMapTexPos);
		// 展示构造的反射图
		//cstShadowMapPass.Run(MainDevCtx).End(MainDevCtx);
		//showShadowMapPass.Run(MainDevCtx).End(MainDevCtx);

		//PPCstShadowMapPass.Run(MainDevCtx).End(MainDevCtx);
		
		// 利用管线构造阴影图
		renderer->ClearRenderTarget(&ShadowMapPos);
		renderer->ClearRenderTarget(&ShadowMapNor);
		renderer->ClearDepthStencil(&ShadowMapDepth2);
		for (UINT i = 0; i < 10; ++i) {
			InstanceData.GetData().x = i;
			ShadowMapViewPortPointer = ShadowMapViewPortssss + i * 10;
			PPCstShadowMapPass2.BindSource(nullptr, ShadowMapViewPortPointer, 10).BindSource(&InstanceData, SBT_GEOMETRY_SHADER, 2);
			PPCstShadowMapPass2.Run(MainDevCtx).End(MainDevCtx);
		}

		// 利用管线展示阴影图
		//renderer->ClearRenderTarget(&ShadowMapDiffuse);
		//renderer->ClearDepthStencil(&ShadowMapDepth2);
		//for (UINT i = 0; i < 10; ++i) {
		//	InstanceData.GetData().x = i;
		//	ShadowMapViewPortPointer = ShadowMapViewPortssss + i * 10;
		//	PPShowShadowMap.BindSource(nullptr, ShadowMapViewPortPointer, 10).BindSource(&InstanceData, SBT_GEOMETRY_SHADER, 2);
		//	PPShowShadowMap.Run(MainDevCtx).End(MainDevCtx);
		//}

		// 展示场景点化结果
		//renderer->ClearRenderTarget(&DepthTexture, { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX });
		//renderer->ClearRenderTarget(&PointsResult, { 0, 0, 0, 0 });
		//showPointsPass.Run(MainDevCtx).End(MainDevCtx);

		// 计算当前的反射结果
		renderer->ClearRenderTarget(&ReflectionResultPos);
		reflectionResultPass.Run(MainDevCtx).End(MainDevCtx);

		showPostProcessPass.Run(MainDevCtx).End(MainDevCtx);

		//showRefPntViewPass.Run(MainDevCtx).End(MainDevCtx);

		renderer->EndRender();
	});
}
