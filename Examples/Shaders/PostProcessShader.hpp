#pragma once
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>
#include <string>
#include <filesystem>

class PostProcessVS : public UnknownVision::VertexShader {
public:
	static std::vector<UnknownVision::VertexAttribute> GetVertexAttributes() {
		return {
			UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
			0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS),
			UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_TEXTURE, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32_FLOAT,
			0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS)
		};
	}
public:
	PostProcessVS() : VertexShader(UnknownVision::FileNameConcatenation(__FILE__, "PostProcessVS.hlsl")) {}
	virtual const char* Name() const { return "PostProcessVS.hlsl"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return {};
	}
};

class CopyToTexturePS : public UnknownVision::PixelShader {
public:
	CopyToTexturePS() : PixelShader(UnknownVision::FileNameConcatenation(__FILE__, "CopyToTexturePS.hlsl")) {}
	virtual const char* Name() const { return "CopyToTexturePS.hlsl"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return {
			{ UnknownVision::ShaderParameterSlotDesc::OnlyReadTexture(0, 1)},
			{ UnknownVision::ShaderParameterSlotDesc::LinearSampler(0, 1) }
		};
	}
};
