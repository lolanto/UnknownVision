#pragma once
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>
#include <GraphicsInterface/RenderDevice.h>
#include <GraphicsInterface/BindingBoard.h>

namespace PBRWithSimpleLight {

	struct BindingBoardSet {
		
	};

	class VS : public UnknownVision::VertexShader {
	public:
		static std::vector<UnknownVision::VertexAttribute> GetVertexAttributes() {
			return {
				UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0, 0, 0),
				UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_NORMAL, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS),
				UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_TANGENT, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS),
				UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_TEXTURE, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32_FLOAT, 0, 0, UnknownVision::VertexAttribute::APPEND_FROM_PREVIOUS)
			};
		}
	public:
		VS() : UnknownVision::VertexShader(UnknownVision::FileNameConcatenation(__FILE__, "PBRWithSimpleLightShaderVS.hlsl")) {}
		virtual ~VS() = default;
		virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const override final {
			return {
				{UnknownVision::ShaderParameterSlotDesc::OnlyReadBuffer(0, 2)}
			};
		}
		const char* Name() const override final { return "PBRWithSimpleLightShaderVS.hlsl"; }
	};

	struct ControlPanel {
		IMath::IFLOAT4 Albedo;
		float Roughness;
		float Metallic;
		float NormalPower;
	};

	class PS : public UnknownVision::PixelShader {
	public:
		PS() : UnknownVision::PixelShader(UnknownVision::FileNameConcatenation(__FILE__, "PBRWithSimpleLightShaderPS.hlsl")) {}
		virtual ~PS() = default;
		const char* Name() const override final { return "PBRWithSimpleLightShaderPS.hlsl"; }
		virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const override final {
			return {
				{UnknownVision::ShaderParameterSlotDesc::OnlyReadBuffer(0, 3), 
				UnknownVision::ShaderParameterSlotDesc::OnlyReadTexture(0, 5)},

				{ UnknownVision::ShaderParameterSlotDesc::LinearSampler(0, 1) }
			};
		}
		std::unique_ptr<UnknownVision::BindingBoard> RequestBindingBoard(size_t idx, RenderDevice* ptrDevice, COMMAND_UNIT_TYPE type = DEFAULT_COMMAND_UNIT) {
			auto&& parameters = GetShaderParameters();
			if (idx >= parameters.size()) {
				LOG_ERROR("Invalid binding board index %d", idx);
				return nullptr;
			}
			size_t numSlots = 0;
			for (const auto& parameterSet : parameters[idx]) {
				numSlots += parameterSet.count;
			}
			std::unique_ptr<UnknownVision::BindingBoard> output(ptrDevice->RequestBindingBoard(numSlots, type));
			return output;
		}
	};
}
