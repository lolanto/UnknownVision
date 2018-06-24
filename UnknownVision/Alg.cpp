#include "Alg.h"
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
#include "Light.h"

#include "UI.h"


const float CUBE_MAP_SIZE = 1280.0f;
const float WIDTH = 480.0f;
const float HEIGHT = 320.0f;

typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT2 float2;

Sampler gLinearSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
Sampler gPointSampler(D3D11_FILTER_MIN_MAG_MIP_POINT);
MeshFactory gMF;

void UITest(DefaultParameters) {
	HRESULT hr;

	UISystem& uiSys = UISystem::GetInstance();
	uiSys.Init();

	BasicWindow bw({ 0, 0, 50, 72 });
	uiSys.Attach(&bw);

	TextCtrl text(L"A", { 0, 0, 50, 72 });
	uiSys.Attach(&text);

	mc->Run([&] {
		uiSys.Draw();
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
