#pragma once
#include <UVUtility.h>
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>
#include <string>
#include <filesystem>
#include <tuple>

#include <Utility/GeneralCamera/GeneralCamera.h>
#include <InfoLog/InfoLog.h>

#include "VertexFactory.h"


template<typename _VertexFactoryType>
class LoadMeshVS : public UnknownVision::VertexShader {
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
	using VertexFactoryType = _VertexFactoryType;
public:
	static const std::vector<UnknownVision::VertexAttribute> GetVertexAttributes() { 
		int a = 0;
		return VertexFactoryType::GetVertexAttributes();
	}

public:
	LoadMeshVS() : UnknownVision::VertexShader(GET_FILE_PATH_REATIVE_TO_THIS_FILE("LoadMeshVS.hlsl")) {}
	virtual const char* Name() const { return "LoadMeshVS.hlsl"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return {
			{ ShaderParameterSlotDesc<0>::GetSlotDesc() }
		};
	}

	virtual bool ModifyShaderCode(std::string& code) const final {
		auto func_replace_str = [](std::string& str, const char* from, const char* to) -> bool {
			size_t start_pos = str.find(from);
			if (start_pos == std::string::npos)
			{
				LOG_ERROR("Cannot find the placement string %s in shader code", from);
				return false;
			}
			str.replace(start_pos, strlen(from), to);
			return true;
		};

		if (!func_replace_str(code, CameraDataBuffer::ShaderCodePlacementString, CameraDataBuffer::ShaderCodeOfBufferDeclaration)) {
			return false;
		}

		if (!func_replace_str(code, VertexFactoryType::GetVertexShaderDeclarationPlacementString(), VertexFactoryType::GetVertexShaderDeclarationCode())) {
			return false;
		}

		return true;
	}
};

using LoadMeshVS_PNT0 = LoadMeshVS<UnknownVision::VertexFactory_PNT0>;

class LoadMeshPS : public UnknownVision::PixelShader {
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
	LoadMeshPS() : PixelShader(GET_FILE_PATH_REATIVE_TO_THIS_FILE("LoadMeshPS.hlsl")) {}
	virtual const char* Name() const { return "LoadMeshPS.hlsl"; }
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
