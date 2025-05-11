#pragma once
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>
#include <string>
#include <filesystem>
#include <tuple>

#include <Utility/GeneralCamera/GeneralCamera.h>
#include <InfoLog/InfoLog.h>


class LoadTextureVS : public UnknownVision::VertexShader {
public:
	
	template<size_t Index>
	struct ShaderParameterSlotDesc {};

	template<>
	struct ShaderParameterSlotDesc<0> {
		UVCameraUtility::GeneralCameraDataStructure CameraData;
		static constexpr const char* ShaderCodePlacementString = "/* #ConstantBuffer0# */";
		static constexpr const char* ShaderCodeOfBufferDeclaration = R"(
cbuffer CameraDataBuffer : register(b0)
{
	GeneralCameraDataStructure CameraData;
};
)";
		static constexpr size_t GetBindingBoardSlotIndex(size_t offset) { return offset + 0; }
		static UnknownVision::ShaderParameterSlotDesc GetSlotDesc() { return UnknownVision::ShaderParameterSlotDesc::OnlyReadBuffer(0, 1); }
	};
	using CameraDataBuffer = ShaderParameterSlotDesc<0>;

	constexpr static size_t TotalShaderParameterSlot = 1;
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
			{ ShaderParameterSlotDesc<0>::GetSlotDesc() }
		};
	}

	virtual bool ModifyShaderCode(std::string& code) const final {
		size_t IndexOfShaderCodePlacementString = code.find(CameraDataBuffer::ShaderCodePlacementString);
		if (IndexOfShaderCodePlacementString == std::string::npos) {
			LOG_ERROR("Cannot find the placement string %s in shader code", CameraDataBuffer::ShaderCodePlacementString);
			return false;
		}
		code.insert(code.find(CameraDataBuffer::ShaderCodePlacementString), CameraDataBuffer::ShaderCodeOfBufferDeclaration);
		return true;
	}
};

class LoadTexturePS : public UnknownVision::PixelShader {
public:
	template<size_t Index>
	struct ShaderParameterSlotDesc {};

	template<>
	struct ShaderParameterSlotDesc<0> {
		static constexpr size_t image_slot = 0;
		static constexpr const char* ShaderCodePlacementString = "/* #TextureBuffer0# */";
		static constexpr const char* ShaderCodeOfBufferDeclaration = R"(
Texture2D<float4> image : register(t0);
)";
		static constexpr size_t GetBindingBoardSlotIndex(size_t offset) { return offset + 0; }
		static UnknownVision::ShaderParameterSlotDesc GetSlotDesc() { return UnknownVision::ShaderParameterSlotDesc::OnlyReadTexture(0, 1); }
	};
	using TextureBuffer = ShaderParameterSlotDesc<0>;

	template<>
	struct ShaderParameterSlotDesc<1> {
		static constexpr size_t GetBindingBoardSlotIndex(size_t offset) { return offset + 1; }
		static UnknownVision::ShaderParameterSlotDesc GetSlotDesc() { return UnknownVision::ShaderParameterSlotDesc::LinearSampler(0, 1); }
	};
	using SamplerBuffer = ShaderParameterSlotDesc<1>;

	constexpr static size_t TotalShaderParameterSlot = 2;
public:
	LoadTexturePS() : PixelShader(FileNameConcatenation(__FILE__, "LoadTexturePS.hlsl")) {}
	virtual const char* Name() const { return "LoadTexturePS.hlsl"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return {
			{ TextureBuffer::GetSlotDesc()},
			{ SamplerBuffer::GetSlotDesc()}
		};
	}

	virtual bool ModifyShaderCode(std::string& code) const final {
		size_t IndexOfShaderCodePlacementString = code.find(TextureBuffer::ShaderCodePlacementString);
		if (IndexOfShaderCodePlacementString == std::string::npos) {
			LOG_ERROR("Cannot find the placement string %s in shader code", TextureBuffer::ShaderCodePlacementString);
			return false;
		}
		code.insert(code.find(TextureBuffer::ShaderCodePlacementString), TextureBuffer::ShaderCodeOfBufferDeclaration);
		return true;
	}
};
