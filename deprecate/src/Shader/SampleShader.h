#pragma once
#include "../RenderSystem/Shader.h"

BEG_NAME_SPACE

class SampleShaderVS : public VertexShader {
public:
	static std::vector<VertexAttribute> GetVertexAttributes() {
		VertexAttribute position = VertexAttribute(VERTEX_ATTRIBUTE_TYPE_POSITION,
			ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
			0, 0, VertexAttribute::APPEND_FROM_PREVIOUS);
		return { position };
	}
public:
	SampleShaderVS() : VertexShader(L"vs.hlsl") {}
};

class SampleShaderPS : public PixelShader {
public:
	SampleShaderPS() : PixelShader(L"ps.hlsl") {}
};

END_NAME_SPACE
