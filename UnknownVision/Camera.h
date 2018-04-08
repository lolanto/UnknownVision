#pragma once
#include <DirectXMath.h>
#include <memory>
#include <wrl.h>
#include "Buffer.h"

typedef unsigned int UINT;
typedef unsigned char byte;

#define VS_CAMERA_DATA_SLOT 2
#define PS_CAMERA_DATA_SLOT 0

class CameraController;

enum MouseButton {
	MBTN_LEFT = 0,
	MBTN_RIGHT
};

enum KeyButton {
	KBTN_W = 0,
	KBTN_A,
	KBTN_S,
	KBTN_D
};

// 摄像机参数说明
struct CAMERA_DESC {
	DirectX::XMFLOAT3						position;
	DirectX::XMFLOAT3						lookAt;
	// 视场宽度单位是弧度制
	float												fov;
	// 视场宽高
	float												width;
	float												height;
	// 近平面
	float												nearPlane;
	// 远平面
	float												farPlane;
	CAMERA_DESC(float width, float height, DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 5.0f },
		DirectX::XMFLOAT3 lkt = { 0.0f, 0.0f, 0.0f },
		float fv = 0.45, float np = 1.0f, float fp = 100.0f);
};

// 摄像机基础数据
class Camera : public UnknownObject {
public:
	Camera(CAMERA_DESC&);

public:
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMFLOAT3 GetLookAt() const;

	DirectX::XMFLOAT4X4 GetViewMat() const;
	DirectX::XMFLOAT4X4 GetProMat() const;

	DirectX::XMFLOAT4X4	GetPrevViewMat() const;
	DirectX::XMFLOAT4X4	GetPrevProjMat() const;

	float GetFOV() const;
	float GetASPECT() const;
	float GetNear() const;
	float GetFar() const;

	void SetPosition(DirectX::XMFLOAT3);
	void SetLookAt(DirectX::XMFLOAT3);
	void SetFOV(float);
	void SetASPECT(float);

private:
	void calcProjMatrix();
	void calcViewMatrix();
private:

	// 影响到view矩阵的参数发生更改
	// 参数包括：摄像机位置，观察方向
	bool																		m_isViewDirty;
	// 影响到projectiong矩阵的参数发生更改
	// 参数包括：fov，长宽
	bool																		m_isProjDirty;

	struct CameraDataStruct {
		DirectX::XMFLOAT4X4										m_viewMatrix;
		DirectX::XMFLOAT4X4										m_viewMatrixInv;
		DirectX::XMFLOAT4X4										m_projMatrix;
		DirectX::XMFLOAT4X4										m_projMatrixInv;
		DirectX::XMFLOAT3											m_pos;
		float temp;
		DirectX::XMFLOAT4											m_param;
	}																			/*m_cameraDataStruct,*/
																				m_cameraPrevData;

	DirectX::XMFLOAT3												m_lookAt;

	float																		m_fov;
	float																		m_aspect;
	float																		m_near;
	float																		m_far;

	ConstantBuffer<CameraDataStruct>					m_buf;
};


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   cubeMap Helper   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
class CubeMapHelper {
public:
	// 输入cubemap的产生的中心位置
	CubeMapHelper(DirectX::XMFLOAT3 pos, float nearZ = 0.1f, float farZ = 50.0f);

	// 获取cubemap的基础view矩阵
	void GetBasicViewMat(DirectX::XMFLOAT4X4*);
	// 获取产生6个面的旋转矩阵以及投影矩阵
	void GetRotAndProjMat(DirectX::XMFLOAT4X4*);

private:
	float																		m_nearZ;
	float																		m_farZ;
private:
	// view matrix
	DirectX::XMMATRIX												m_viewMat;
	// projection matrix
	DirectX::XMMATRIX												m_projectMat;
	// rotate around y axis about 90 deg
	DirectX::XMMATRIX												m_leftMat;
	// rotate around y axis about -90 deg
	DirectX::XMMATRIX												m_rightMat;
	// rotate around x axis about -90 deg
	DirectX::XMMATRIX												m_topMat;
	// rotate around x axis about 90 deg
	DirectX::XMMATRIX												m_bottomMat;
	// rotate around y axis about 180 deg
	DirectX::XMMATRIX												m_backMat;
	// front face, don't need to rotate
	DirectX::XMMATRIX												m_frontMat;
};

// 摄像机控制器接口
class CameraController {
public:
	CameraController(Camera*);
	// 输入最新鼠标屏幕位置
	virtual void MouseMove(float x, float y) = 0;
	// 处理鼠标点击事件
	virtual void MouseEventHandler(MouseButton btn, bool state) = 0;
	// 处理键盘事件
	virtual void KeyEventHandler(KeyButton btn, bool state) = 0;
protected:
	Camera*																m_camera;
};

class OrbitController : public CameraController {
public:
	OrbitController(Camera*);
public:
	void MouseMove(float x, float y);
	void MouseWheel(float);
	void MouseEventHandler(MouseButton btn, bool state);
	void KeyEventHandler(KeyButton btn, bool state);
private:
	DirectX::XMVECTOR												m_left;
	DirectX::XMVECTOR												m_up;
	DirectX::XMVECTOR												m_forward;
	DirectX::XMVECTOR												m_pos;
	// 鼠标上次的屏幕位置
	DirectX::XMFLOAT2												m_mouseLastPos;
	// 鼠标按键状态
	bool																		m_isLeftDown;
	bool																		m_isRightDown;
};
