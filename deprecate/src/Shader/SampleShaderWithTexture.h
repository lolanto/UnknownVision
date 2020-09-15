#pragma once
#include "../RenderSystem/Shader.h"
#include "../RenderSystem/Pipeline.h"

BEG_NAME_SPACE

class SampleShaderWithTextureVS : public VertexShader {
public:
	static std::vector<VertexAttribute> GetVertexAttributes() {
		return {
			VertexAttribute(VERTEX_ATTRIBUTE_TYPE_POSITION, ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0, 0, VertexAttribute::APPEND_FROM_PREVIOUS),
			VertexAttribute(VERTEX_ATTRIBUTE_TYPE_TEXTURE, ELEMENT_FORMAT_TYPE_R32G32_FLOAT, 0, 0, VertexAttribute::APPEND_FROM_PREVIOUS)
		};
	}
public:
	SampleShaderWithTextureVS() : VertexShader(L"vs_texture.hlsl") {}
	virtual std::vector<std::vector<ShaderParameterSlotDesc>> GetShaderParameters() const override final {
		return {
			{ShaderParameterSlotDesc::OnlyReadBuffer(0, 1)}
		};
	}
};

class SampleShaderWithTexturePS : public PixelShader {
public:
	SampleShaderWithTexturePS() : PixelShader(L"ps_texture.hlsl") {}
	virtual std::vector<std::vector<ShaderParameterSlotDesc>> GetShaderParameters() const override final {
		return {
			{ShaderParameterSlotDesc::OnlyReadTexture(0, 1)},
			{ShaderParameterSlotDesc::LinearSampler(0)}
		};
	}
};

END_NAME_SPACE
