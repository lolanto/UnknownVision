#pragma once
#include "UnknownObject.h"
#include <DirectXMath.h>
// ÿ����Ⱦpass�ĹǼ�
/*
1. �������͵�pass
	ֻ����compute shader��ֻ��Ҫ�����ļ�������
2. ��Ⱦ���͵�pass
	���ٰ���VS, PS��pass��������л������Ⱦ

��������
1. ����Դ
2. ��shader
3. ����pass
4. ��ֹ����
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
class RasterState;

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
	virtual BasePass& BindSource(IConstantBuffer*, ShaderBindTarget, SIZE_T);
	virtual BasePass& BindSource(ITexture*, ShaderBindTarget, SIZE_T);
	virtual BasePass& BindSource(IBuffer*, ShaderBindTarget, SIZE_T);
	virtual BasePass& BindSource(ISamplerState*, ShaderBindTarget, SIZE_T);
	virtual BasePass& Run(ID3D11DeviceContext*) = 0;
	virtual BasePass& End(ID3D11DeviceContext*) = 0;
protected:
	std::vector<BindingData>							m_bindingData;
};

class ShadingPass : public BasePass {
public:
	static BindingData													Def_RasterState;
	static D3D11_VIEWPORT											Def_ViewPort;
public:
	ShadingPass(VertexShader*, PixelShader* ps = nullptr, GeometryShader* gs = nullptr);
	using BasePass::BindSource;
	ShadingPass& BindSource(IRenderTarget*, bool beforeClear, bool afterClear);
	ShadingPass& BindSource(IDepthStencil*, bool beforeClear, bool afterClear);
	ShadingPass& BindSource(Mesh*, ShaderBindTarget sbt = SBT_UNKNOWN, SIZE_T slot = -1);
	ShadingPass& BindSource(RasterState* rs = nullptr, D3D11_VIEWPORT* vs = nullptr);
	ShadingPass& BindSource(UnknownObject*, ShaderBindTarget, SIZE_T);

	ShadingPass& Run(ID3D11DeviceContext*);
	ShadingPass& End(ID3D11DeviceContext*);
private:
	std::vector<ID3D11RenderTargetView*>					m_rtvs;
	std::vector<ClearScheme>											m_rtvsClearSchemes;

	IDepthStencil*																m_ds;
	ClearScheme																m_dsClearScheme;

	// ģ�Ͱ���Ϣ������ʽ��������ִ�и�pass
	BindingData																	m_meshBindingData;
	// ���ù�դ״̬������ʽ���������Ⱦ��Ĭ�Ϲ�դ����
	BindingData																	m_rasterStateData;
	// ����view port״̬
	D3D11_VIEWPORT*														m_viewport;

	VertexShader*																m_vs;
	PixelShader*																	m_ps;
	GeometryShader*														m_gs;
};

class ComputingPass : public BasePass {
public:
	ComputingPass(ComputeShader*, DirectX::XMFLOAT3);
	ComputingPass& Run(ID3D11DeviceContext*);
	ComputingPass& End(ID3D11DeviceContext*);
private:
	DirectX::XMFLOAT3										m_groups;
	ComputeShader*											m_cs;
};
