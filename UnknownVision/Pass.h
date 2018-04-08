#pragma once
#include "UnknownObject.h"
#include <DirectXMath.h>


#define ITERATE_BINDING_DATA(devCtx, bd) for (auto& iter : bd) { \
	iter.resPointer->Bind(devCtx, iter.bindTarget, iter.slot); \
} \

#define ITERATE_UNBINDING_DATA(devCtx, bd) for (auto& iter : bd) { \
	iter.resPointer->Unbind(devCtx, iter.bindTarget, iter.slot); \
} \

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
class IUnorderAccess;
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

template<typename T>
struct BindingData {
	T*								resPointer;
	ShaderBindTarget	bindTarget;
	SIZE_T						slot;
	BindingData(T* rp = nullptr, ShaderBindTarget sbt = SBT_UNKNOWN, SIZE_T slot = -1);
};

template<typename T>
BindingData<T>::BindingData(T* rp,
	ShaderBindTarget sbt, SIZE_T slot): resPointer(rp), bindTarget(sbt), slot(slot) {}

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
	virtual BasePass& BindSource(IUnorderAccess*, ShaderBindTarget, SIZE_T);
	virtual BasePass& BindSource(UnknownObject*, ShaderBindTarget, SIZE_T);
	virtual BasePass& Run(ID3D11DeviceContext*);
	virtual BasePass& End(ID3D11DeviceContext*);
protected:
	std::vector<BindingData<IConstantBuffer>>					m_bdOfConstBuffer;
	std::vector<BindingData<ITexture>>								m_bdOfTexture;
	std::vector<BindingData<IBuffer>>								m_bdOfBuffer;
	std::vector<BindingData<ISamplerState>>					m_bdOfSamplerState;
	std::vector<BindingData<IUnorderAccess>>					m_bdOfUnorderAccess;
	std::vector<BindingData<UnknownObject>>					m_bdOfUnknownObject;
};

class ShadingPass : public BasePass {
public:
	static BindingData<RasterState>								Def_RasterState;
	static D3D11_VIEWPORT											Def_ViewPort;
public:
	ShadingPass(VertexShader*, PixelShader* ps = nullptr, GeometryShader* gs = nullptr);
	using BasePass::BindSource;
	ShadingPass& BindSource(IRenderTarget*, bool beforeClear, bool afterClear);
	ShadingPass& BindSource(IDepthStencil*, bool beforeClear, bool afterClear);
	ShadingPass& BindSource(Mesh*, ShaderBindTarget sbt = SBT_UNKNOWN, SIZE_T slot = -1);
	ShadingPass& BindSource(RasterState* rs = nullptr, D3D11_VIEWPORT* vs = nullptr);

	ShadingPass& Run(ID3D11DeviceContext*);
	ShadingPass& End(ID3D11DeviceContext*);
private:
	std::vector<ID3D11RenderTargetView*>					m_rtvs;
	std::vector<ClearScheme>											m_rtvsClearSchemes;

	IDepthStencil*																m_ds;
	ClearScheme																m_dsClearScheme;

	// ģ�Ͱ���Ϣ������ʽ��������ִ�и�pass
	BindingData<Mesh>													m_meshBindingData;
	// ���ù�դ״̬������ʽ���������Ⱦ��Ĭ�Ϲ�դ����
	BindingData	<RasterState>											m_rasterStateData;
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

