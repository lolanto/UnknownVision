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
#include "IMeshResource.h"
#include "Viewport.h"
#include "TMeshPass.h"

template<typename _VertexFactoryType>
class LoadMeshVS : public UnknownVision::VertexShader {
public:
	using SlotTrait = TShaderParameterSlotTrait<Viewport::ShaderParameter<0>>;
	using ViewportParameter = SlotTrait::SlotType<0>::Type;
	constexpr static size_t TotalShaderParameterSlot = SlotTrait::TotalShaderParameterSlot;
	static std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParametersStatic() {
		return SlotTrait::GetSlotDesc();
	}
public:
	using VertexFactoryType = _VertexFactoryType;
public:
	static const std::vector<UnknownVision::VertexAttribute> GetVertexAttributes() {  return VertexFactoryType::GetVertexAttributes(); }
public:
	LoadMeshVS() : UnknownVision::VertexShader(GET_FILE_PATH_REATIVE_TO_THIS_FILE("LoadMesh.hlsl")) {}
	virtual const char* Name() const { return "LoadMesh_mainVS.hlsl"; }
	virtual const char* GetEntranceName() const override final { return "mainVS"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return SlotTrait::GetSlotDesc();
	}

	virtual bool ModifyShaderCode(std::string& code) const final {
		SlotTrait::LoopShaderModificationWithCallback([&code](const char* replacementString, const char* shaderCode)
			{
				if (!ReplaceShaderSourceCode(code, replacementString, shaderCode))
				{
					LOG_ERROR("Failed to replace shader code for %s", replacementString);
					abort();
				}
			});

		if (!ReplaceShaderSourceCode(code, VertexFactoryType::GetVertexShaderDeclarationPlacementString(), VertexFactoryType::GetVertexShaderDeclarationCode())) {
			return false;
		}
		return true;
	}
};

class LoadMeshPS : public UnknownVision::PixelShader {
public:
	struct TextureBuffer {
		static constexpr const char* GetShaderCodePlacementString() noexcept { return "/* #TextureBuffer0# */"; }
		static const char* GetShaderCode() { 
			return R"(
				Texture2D<float4> image : register(t0);
				)";
		}
		static constexpr size_t GetBindingBoardSlotIndex(size_t offset) { return offset + 0; }
		static std::vector<UnknownVision::ShaderParameterSlotDesc> GetSlotDesc() {
			return { UnknownVision::ShaderParameterSlotDesc::OnlyReadTexture(0, 1) };
		}
	};

	using SlotTrait = TShaderParameterSlotTrait<TextureBuffer>;
	constexpr static size_t TotalShaderParameterSlot = SlotTrait::TotalShaderParameterSlot;

	static std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParametersStatic() {
		return SlotTrait::GetSlotDesc();
	}
public:
	struct TextureSampler {
		static UnknownVision::ShaderParameterSlotDesc GetSlotDesc() { return UnknownVision::ShaderParameterSlotDesc::LinearSampler(0, 1); }
	};
public:
	LoadMeshPS() : PixelShader(GET_FILE_PATH_REATIVE_TO_THIS_FILE("LoadMesh.hlsl")) {}
	virtual const char* Name() const { return "LoadMesh_mainPS.hlsl"; }
	virtual const char* GetEntranceName() const override final { return "mainPS"; }
	virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const {
		return SlotTrait::GetSlotDesc();
	}
	virtual std::vector<UnknownVision::ShaderParameterSlotDesc> GetSamplerParameters() const {
		return { TextureSampler::GetSlotDesc() };
	}

	virtual bool ModifyShaderCode(std::string& code) const final {
		SlotTrait::LoopShaderModificationWithCallback([&code](const char* replacementString, const char* shaderCode)
			{
				if (!ReplaceShaderSourceCode(code, replacementString, shaderCode))
				{
					LOG_ERROR("Failed to replace shader code for %s", replacementString);
					abort();
				}
			}
		);
		return true;
	}
};


class MeshPass : public TMeshPass<LoadMeshVS<UnknownVision::VertexFactory_PNT0>, LoadMeshPS> {
public:
	using BaseType = TMeshPass<LoadMeshVS<UnknownVision::VertexFactory_PNT0>, LoadMeshPS>;
	using VertexShaderType = typename BaseType::VertexShaderType;
	using PixelShaderType = typename BaseType::PixelShaderType;
public:
	bool Init(UnknownVision::CommandUnit* cmdUnit
		, UnknownVision::RenderDevice* renderDevice
		, UnknownVision::RenderBackend* renderBackend)
	{
		if (BaseType::Init(cmdUnit, renderDevice, renderBackend) == false)
			return false;
		m_texture = nullptr;
		m_vertexBuffersHolder = nullptr;
		return true;
	}

	// TODO: 这个函数目前需要手动关联Binding Board以及资源关系
	bool Update(UnknownVision::VertexBuffersHolder* vertexBuffersHolder
		, UnknownVision::Texture2D* texture
		, const UnknownVision::GPUBuffer* viewportConstantBuffer)
	{
		m_vertexBuffersHolder = vertexBuffersHolder;

		bool isNeedToUpdateBindingBoardForVS = false;
		if (viewportConstantBuffer != nullptr && viewportConstantBuffer != m_viewportConstantBuffer)
		{
			isNeedToUpdateBindingBoardForVS = true;
			m_viewportConstantBuffer = viewportConstantBuffer;
		}

		if (isNeedToUpdateBindingBoardForVS)
		{
			if (!m_bindingBoardForVS[0]->IsEnableBinding())
				m_bindingBoardForVS[0]->Reset();
			m_bindingBoardForVS[0]->BindingResource(
				VertexShaderType::ViewportParameter::GetSlotDesc()[0].slot,
				m_viewportConstantBuffer, VertexShaderType::ViewportParameter::GetSlotDesc()[0].paramType);
			m_bindingBoardForVS[0]->Close();
		}

		bool isNeedToUpdateBindingBoardForPS = false;
		if (texture != nullptr && texture != m_texture)
		{
			isNeedToUpdateBindingBoardForPS = true;
			m_texture = texture;
		}
		if (isNeedToUpdateBindingBoardForPS)
		{
			if (!m_bindingBoardForPS[0]->IsEnableBinding())
				m_bindingBoardForPS[0]->Reset();
			m_bindingBoardForPS[0]->BindingResource(
				PixelShaderType::TextureBuffer::GetSlotDesc()[0].slot, 
				m_texture, PixelShaderType::TextureBuffer::GetSlotDesc()[0].paramType);
			
			m_bindingBoardForPS[0]->Close();
		}
		
		return true;
	}

	bool Draw(UnknownVision::GPUResource** renderTargets, size_t renderTargetCount
		, UnknownVision::GPUResource* depthStencilBuffer
		, UnknownVision::CommandUnit* cmdUnit
		, const UnknownVision::ViewportDesc& vp, const UnknownVision::ScissorRectDesc& sr)
	{
		if (!BaseType::Draw(renderTargets, renderTargetCount, depthStencilBuffer, cmdUnit, vp, sr))
			abort();
		m_vertexBuffersHolder->BindingFunction(m_vertexBuffersHolder, cmdUnit);
		cmdUnit->Draw(0, m_vertexBuffersHolder->IndexCount, 0);
		return true;
	}
private:
	UnknownVision::Texture2D* m_texture;
	UnknownVision::VertexBuffersHolder* m_vertexBuffersHolder;
	const UnknownVision::GPUBuffer* m_viewportConstantBuffer;
};
