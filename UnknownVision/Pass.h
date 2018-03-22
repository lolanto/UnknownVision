#pragma once
#include "UnknownObject.h"
#include <DirectXMath.h>
// 每个渲染pass的骨架
/*
1. 计算类型的pass
	只包含compute shader，只需要负责尽心计算任务
2. 渲染类型的pass
	至少包含VS, PS的pass，负责进行画面的渲染

动作抽象：
1. 绑定资源
2. 绑定shader
3. 运行pass
4. 终止运行
*/

// Basic Resource!
class IConstantBuffer;
class ITexture;
class IBuffer;
class ISamplerState;
class IRenderTarget;
class IDepthStencil;
// Shaders
class VertexShader;
class PixelShader;
class GeometryShader;
class ComputeShader;
// Concrete Resource!
class Camera;
class Mesh;
class Light;

enum SourceType {
	MST_INVALID_TYPE = 0,
	MST_CONSTANT_BUFFER,
	MST_TEXTURE,
	MST_BUFFER,
	MST_SAMPLER_STATE,
	MST_UNKNOWN_OBJECT
};

struct BindingData {
	union sourceType
	{
		IConstantBuffer* constBuf;
		ITexture*				tex;
		IBuffer*				buf;
		ISamplerState*		sampler;
		UnknownObject*	object;
	}								resPointer;
	SourceType				resType;
	ShaderBindTarget	bindTarget;
	SIZE_T						slot;
	BindingData(SourceType st = MST_INVALID_TYPE,
		ShaderBindTarget sbt = SBT_UNKNOWN, SIZE_T slot = -1);
};

struct ClearScheme {
	// clear the render target before starting rendering
	bool							bc;
	// clear the render target after rendering
	bool							ac;
	ClearScheme(bool bc = false, bool ac = false);
};

class BasePass {
public:
	// sourceType, bind target, slot
	virtual void BindSource(IConstantBuffer*, ShaderBindTarget, SIZE_T);
	virtual void BindSource(ITexture*, ShaderBindTarget, SIZE_T);
	virtual void BindSource(IBuffer*, ShaderBindTarget, SIZE_T);
	virtual void BindSource(ISamplerState*, ShaderBindTarget, SIZE_T);
	virtual void Run(ID3D11DeviceContext*) = 0;
	virtual void End(ID3D11DeviceContext*) = 0;
protected:
	std::vector<BindingData>							m_bindingData;
};

class ShadingPass : public BasePass {
public:
	ShadingPass(VertexShader*, PixelShader*, GeometryShader* gs = nullptr);
	void BindSource(IRenderTarget*, bool beforeClear, bool afterClear);
	void BindSource(IDepthStencil*, bool beforeClear, bool afterClear);
	void BindSource(Mesh*);
	void BindSource(UnknownObject*, ShaderBindTarget, SIZE_T);

	void Run(ID3D11DeviceContext*);
	void End(ID3D11DeviceContext*);
private:
	std::vector<ID3D11RenderTargetView*>					m_rtvs;
	std::vector<ClearScheme>											m_rtvsClearSchemes;

	IDepthStencil*																m_ds;
	ClearScheme																m_dsClearScheme;

	BindingData																	m_meshBindingData;

	VertexShader*																m_vs;
	PixelShader*																	m_ps;
	GeometryShader*														m_gs;
};

class ComputingPass : public BasePass {
public:
	ComputingPass(ComputeShader*, DirectX::XMFLOAT3);
	void Run(ID3D11DeviceContext*);
	void End(ID3D11DeviceContext*);
private:
	DirectX::XMFLOAT3										m_groups;
	ComputeShader*											m_cs;
};
