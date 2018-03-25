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

const float WIDTH = 1280.0f;
const float HEIGHT = 960.0f;

const float CUBE_MAP_SIZE = 1280.0f;

typedef DirectX::XMFLOAT4 float4;

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
	cc.Setup(GetMainDev);
	// 添加摄像机控制器
	OrbitController obController(&cc);
	CameraControllerSetting(obController);

	gLinearSampler.Setup(GetMainDev);
	gPointSampler.Setup(GetMainDev);

	// Shaders
	VertexShader IBLVS("../Debug/imageBasedLightingVS.cso");
	IBLVS.Setup(GetMainDev);
	PixelShader IBLPS("../Debug/imageBasedLightingPS.cso");
	IBLPS.Setup(GetMainDev);

	VertexShader IBLBasicVS("../Debug/imageBasedLightingBasicVS.cso");
	IBLBasicVS.Setup(GetMainDev);
	PixelShader IBLBasicPS("../Debug/imageBasedLightingBasicPS.cso");
	IBLBasicPS.Setup(GetMainDev);

	// Model
	Model basicModel;
	basicModel.Setup(GetMainDev);

	Model iblModel;
	DirectX::XMFLOAT3 dir = { 0.0f, 2.0f, 3.0f };
	DirectX::XMFLOAT3 rag = { 0.0f, 0.2f, 0.0f };
	iblModel.Translate(dir);
	iblModel.RotateAroundOrigin(rag);
	iblModel.Setup(GetMainDev);
	
	struct iblMatrixStruct {
		DirectX::XMFLOAT4X4 matrix;
	};

	struct iblWidthHeight {
		DirectX::XMFLOAT4 wh;
	};

	ConstantBuffer<iblMatrixStruct>  iblMatrixCB;
	iblMatrixCB.GetData().matrix = iblModel.GetModelData().modelMatrixInv;
	iblMatrixCB.Setup(GetMainDev);
	ConstantBuffer<iblWidthHeight> iblWidthHeightCB;
	iblWidthHeightCB.GetData().wh = { 1.0f, 1.0f, 0.0f, 0.0f };
	iblWidthHeightCB.Setup(GetMainDev);

	// Mesh
	std::vector<std::shared_ptr<Mesh>> meshList;
	meshList = gMF.Load("./ImageBasedLighting/testEnv.obj");
	for (auto iter = meshList.begin(), end = meshList.end(); iter != end; ++iter) {
		iter->get()->Setup(GetMainDev);
	}

	std::shared_ptr<Mesh> plane;
	gMF.Load(BMT_PLANE, plane, 1.0f, 1.0f);
	plane.get()->Setup(GetMainDev);

	// texture
	CommonTexture BC(L"./ImageBasedLighting/AO.png");
	BC.Setup(GetMainDev);
	CommonTexture refImage(L"./ImageBasedLighting/based.jpg");
	refImage.Setup(GetMainDev);

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
		pass0.Run(GetMainDevCtx);
		pass0.End(GetMainDevCtx);
		pass1.Run(GetMainDevCtx);
		pass1.End(GetMainDevCtx);
		renderer->EndRender();
	});

}