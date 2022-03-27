#pragma once
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>
#include <string>
#include <filesystem>

class TAAPS : public UnknownVision::PixelShader {
public:
	TAAPS() : PixelShader(UnknownVision::FileNameConcatenation(__FILE__, "TAAPS.hlsl")) {}
	virtual const char* Name() const { return "TAAPS.hlsl"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return {
			{ UnknownVision::ShaderParameterSlotDesc::OnlyReadTexture(0, 2) },
			{ UnknownVision::ShaderParameterSlotDesc::PointSampler(0, 1)},
		};
	}
};