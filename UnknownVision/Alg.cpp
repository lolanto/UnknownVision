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

const float WIDTH = 960.0f;
const float HEIGHT = 960.0f;

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
		.BindSource(&cc, SBT_PIXEL_SHADER, 0)
		.BindSource(renderer->GetMainRT(), true, false)
		.BindSource(renderer->GetMainDS(), true, false)
		.BindSource(&ltc_mat, SBT_PIXEL_SHADER, 0)
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
		.BindSource(&cc, SBT_PIXEL_SHADER, 0)
		.BindSource(renderer->GetMainRT(), false, false)
		.BindSource(renderer->GetMainDS(), false, false)
		.BindSource(&glass, SBT_PIXEL_SHADER, 0)
		.BindSource(&gLinearSampler, SBT_PIXEL_SHADER, 0);

	mc->Run([&] {
		pass0.Run(MainDevCtx).End(MainDevCtx);
		pass1.Run(MainDevCtx).End(MainDevCtx);
		renderer->EndRender();
	});
}