#pragma once
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>
#include <string>
#include <filesystem>

class LoadTextureVS : public UnknownVision::VertexShader {
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
	LoadTextureVS() : VertexShader(FileNameConcatenation(__FILE__, "LoadTextureVS.hlsl")) {}
	virtual const char* Name() const { return "LoadTextureVS.hlsl"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const { 
		return {
			{ UnknownVision::ShaderParameterSlotDesc::OnlyReadBuffer(0, 1)}
		};
	}
};

class LoadTexturePS : public UnknownVision::PixelShader {
public:
	LoadTexturePS() : PixelShader(FileNameConcatenation(__FILE__, "LoadTexturePS.hlsl")) {}
	virtual const char* Name() const { return "LoadTexturePS.hlsl"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return {
			{ UnknownVision::ShaderParameterSlotDesc::OnlyReadTexture(0, 1)},
			{ UnknownVision::ShaderParameterSlotDesc::LinearSampler(0, 1) }
		};
	}
};
