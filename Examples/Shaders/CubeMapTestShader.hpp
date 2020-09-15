#pragma once
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>

namespace CubeMapTestShader {
	class VS : public UnknownVision::VertexShader {
	public:
		static std::vector<UnknownVision::VertexAttribute> GetVertexAttributes() {
			return {
				UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
				0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS),
				UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_NORMAL, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
				0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS)
			};
		}
		VS() : VertexShader(UnknownVision::FileNameConcatenation(__FILE__, "CubeMapTestVS.hlsl")) {}
		virtual ~VS() = default;
		virtual const char* Name() const { return "CubeMapTestVS.hlsl"; }
		virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
			return {
				{ UnknownVision::ShaderParameterSlotDesc::OnlyReadBuffer(0, 1)}
			};
		}
	};

	class PS : public UnknownVision::PixelShader {
	public:
		PS() : PixelShader(UnknownVision::FileNameConcatenation(__FILE__, "CubeMapTestPS.hlsl")) {}
		virtual ~PS() = default;
		virtual const char* Name() const { return "CubeMapTestPS.hlsl"; }
		virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
			return {
				{ UnknownVision::ShaderParameterSlotDesc::OnlyReadTexture(0, 1)},
				{ UnknownVision::ShaderParameterSlotDesc::LinearSampler(0, 1) }
			};
		}
	};
}
