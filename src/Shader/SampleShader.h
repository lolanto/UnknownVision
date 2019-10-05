#pragma once
#include "../RenderSystem/Shader.h"

BEG_NAME_SPACE

inline std::vector<VertexAttribute> SampleVertexAttributes() {
	VertexAttribute position = VertexAttribute(VERTEX_ATTRIBUTE_TYPE_POSITION,
		ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
		0, 0, VertexAttribute::APPEND_FROM_PREVIOUS);
	return { position };
}

class SampleShaderVS : public VertexShader {
public:
	SampleShaderVS() : VertexShader(L"vs.hlsl") {}

	virtual  std::vector<ParameterPackageInterface*> Pack() const override final {
		return {};
	}
};

class SampleShaderPS : public PixelShader {
public:
	SampleShaderPS() : PixelShader(L"ps.hlsl") {}

	virtual std::vector<ParameterPackageInterface*> Pack() const override final {
		return {};
	}
};

END_NAME_SPACE
