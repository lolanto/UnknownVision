#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <d3d11.h>
#include <wrl.h>

#include "UnknownObject.h"
#include "Buffer.h"

class ITexture;
class PixelShader1;
class VertexShader1;
class GeometryShader1;
class Sampler;
class StructuredBuffer;

struct ConstantBufferState {
	ConstantBuffer*									buffer;
	ShaderBindTarget									target;
	UINT													slot;
	ConstantBufferState(ConstantBuffer* cb = NULL,
		ShaderBindTarget sbt = SBT_UNKNOWN, UINT slot = -1);
};

struct TextureState {
	ITexture*												texture;
	ShaderBindTarget									target;
	UINT													slot;
	TextureState(ITexture* it = NULL,
		ShaderBindTarget sbt = SBT_UNKNOWN, UINT slot = -1);
};

struct SamplerState {
	Sampler*												sampler;
	ShaderBindTarget									target;
	UINT													slot;
	SamplerState(Sampler* sa = NULL,
		ShaderBindTarget sbt = SBT_UNKNOWN, UINT slot = -1);
};

class Material : public UnknownObject{
public:
	Material();
public:
	bool Setup(ID3D11Device*);
	void Bind(ID3D11DeviceContext*);
	void Unbind(ID3D11DeviceContext*);

	bool SetVertexShader(VertexShader1*);
	bool SetPixelShader(PixelShader1*);
	bool SetGeometryShader(GeometryShader1*);

	bool SetConstantBuffer(ConstantBuffer*, ShaderBindTarget, UINT);
	bool RemoveConstantBuffer(ConstantBuffer*);

	// 更换texture直接设置成同一个Shader同一个slot即可
	bool SetTexture(ITexture*, ShaderBindTarget, UINT);
	bool RemoveTexture(ITexture*);

	bool SetSamplerState(Sampler*, ShaderBindTarget, UINT);
	bool RemoveSamplerState(Sampler*);

private:
	// 检查当前material是否可用
	bool checkEnable();
	void removeVertexShader();
	void removePixelShader();
	void removeGeometryShader();
private:
	std::vector<ConstantBufferState>							m_bufStates;
	std::vector<TextureState>										m_texStates;
	std::vector<SamplerState>										m_samplerStates;

	std::vector<ParamIOLayout>*									m_inputInterface;
	std::vector<ParamIOLayout>*									m_outputInterface;
	VertexShader1*														m_vertexShader;
	PixelShader1*														    m_pixelShader;
	GeometryShader1*													m_geometryShader;
	bool																			m_enable;
};
