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
	Canvas tracingResult(WIDTH, HEIGHT);
	tracingResult.SetUARes();
	tracingResult.Setup(MainDev);

	// 充当计数器的结构化缓冲区
	StructuredBuffer<int, 1> structureCounter(false);
	structureCounter.Setup(MainDev);

	struct LinkedListData {
		DirectX::XMFLOAT4 data;
	};
	ConstantBuffer<LinkedListData> linkedListData;
	linkedListData.Setup(MainDev);
	linkedListData.GetData().data = { WIDTH, HEIGHT * 4,  WIDTH * HEIGHT * 4, HEIGHT };

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

	// 对构建的屏幕信息进行整理
	ComputeShader LLCS("../Debug/LinkedListArrangeCS.cso");
	LLCS.Setup(MainDev);

	// 构建多个level
	ComputeShader FilterCS("../Debug/MultiLevelCS.cso");
	FilterCS.Setup(MainDev);

	// 光线追踪结果
	ComputeShader TracingCS("../Debug/ScreenSpaceReflectionCS.cso");
	TracingCS.Setup(MainDev);

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
		.BindSource(&linkedListData, SBT_PIXEL_SHADER, 1)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	ComputingPass pass1(&LLCS, { 96, 96 ,1 });
	pass1
		.BindSourceTex(&fragmentColor, SBT_COMPUTE_SHADER, 0)
		.BindSourceTex(&fragmentHead, SBT_COMPUTE_SHADER, 1)
		.BindSourceTex(&fragmentDepthAndNext, SBT_COMPUTE_SHADER, 2)

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
		.BindSourceUA(&tracingResult, SBT_COMPUTE_SHADER, 0)
		.BindSource(&cc, SBT_COMPUTE_SHADER, 0);

	ShadingPass passShow(&QuadVS, &QuadPS);
	passShow
		.BindSource(plane.get())
		.BindSource(renderer->GetMainRT(), true, false)
		.BindSource(&basicModel, SBT_VERTEX_SHADER, VS_MODEL_DATA_SLOT)
		.BindSourceTex(&tracingResult, SBT_PIXEL_SHADER, 0)
		.BindSource(&gPointSampler, SBT_PIXEL_SHADER, 0);

	mc->Run([&] {
		passBasic.Run(MainDevCtx).End(MainDevCtx);

		renderer->ClearRenderTarget(&fragmentColor);
		renderer->ClearRenderTarget(&fragmentHead);
		renderer->ClearRenderTarget(&fragmentDepthAndNext);
		renderer->ClearRenderTarget(&fragmentDiffuse);
		renderer->ClearRenderTarget(&fragmentDepth1);
		renderer->ClearRenderTarget(&fragmentDepth2);
		renderer->ClearRenderTarget(&fragmentDepth3);
		renderer->ClearRenderTarget(&fragmentDepth4);
		renderer->ClearRenderTarget(&fragmentDepth5);
		renderer->ClearRenderTarget(&tracingResult);

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

		passTracing.Run(MainDevCtx).End(MainDevCtx);

		passShow.Run(MainDevCtx).End(MainDevCtx);

		renderer->EndRender();
	});
}