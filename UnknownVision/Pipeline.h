#pragma once
#include "UnknownObject.h"
#include <DirectXMath.h>

class Mesh;
class Material;
class IRenderTarget;
class IUATarget;
class ITexture;
class Camera;
class ComputeShader1;

struct RenderTargetState {
	IRenderTarget*						rt;
	bool										beforeClear;
	bool										afterClear;
	RenderTargetState(IRenderTarget*, bool bc = false, bool ac = false);
};

// 绑定在cs上的2D贴图的绑定信息
struct CSTextureState {
	ITexture*								tex;
	UINT									slot;
	CSTextureState(ITexture*, UINT);
};

// 绑定在cs上的UA贴图信息
struct CSUAState {
	IUATarget	*							uat;
	UINT									slot;
	CSUAState(IUATarget*, UINT);
};

class Pipeline : public UnknownObject {
public:
	Pipeline();
public:
	// 创建当前的渲染对象的数组
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);

	void AddRenderTarget(IRenderTarget*, bool bc = false, bool ac = false);
	void RemoveRenderTarget(IRenderTarget*);
	void SetDepthStencilView(ID3D11DepthStencilView*, bool bc = false, bool ac = false);

	bool SetMaterial(Material*);
	void AddMesh(Mesh*);
	void RemoveMesh(Mesh*);

	void SetCamera(Camera*);

private:
	std::vector<RenderTargetState>				m_interfaceRTs;
	std::vector<ID3D11RenderTargetView*>	m_RTs;
	ID3D11DepthStencilView*						m_DS;
	// before rendering clear depth stencil buffer
	bool															m_bcDS;
	// after rendering clear depth stencil buffer
	bool															m_acDS;

	Material*													m_material;
	std::vector<Mesh*>									m_mesh;
	Camera*													m_camera;
};

class ComputePipeline : public UnknownObject {
public:
	ComputePipeline(ComputeShader1*, DirectX::XMUINT3);
public:
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);

	void BindResource(ITexture*, UINT);
	void BindUATarget(IUATarget*, UINT);
private:
	DirectX::XMUINT3									m_group;
	std::vector<CSTextureState>					m_texStates;
	std::vector<CSUAState>							m_uaStates;
	ComputeShader1*									m_computeShader;
};
