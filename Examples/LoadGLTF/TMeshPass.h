#pragma once

#include <UVConfig.h>
#include <GraphicsInterface/RenderBackend.h>
#include <GraphicsInterface/RenderDevice.h>
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/BindingBoard.h>
#include <GraphicsInterface/CommandUnit.h>
#include <memory>

template<typename TypeOfVS, typename TypeOfPS>
class TMeshPass {
public:
	using VertexShaderType = TypeOfVS;
	using PixelShaderType = TypeOfPS;
public:
	bool Init(UnknownVision::CommandUnit* cmdUnit
		, UnknownVision::RenderDevice* renderDevice
		, UnknownVision::RenderBackend* renderBackend)
	{
		if (!renderBackend || !cmdUnit || !renderDevice) {
			LOG_ERROR("Invalid parameters for MeshPass initialization.");
			return false;
		}
		m_vertexShader = std::make_unique<VertexShaderType>();
		m_pixelShader = std::make_unique<PixelShaderType>();
		if (!renderBackend->InitializeShaderObject(m_vertexShader.get()) || !renderBackend->InitializeShaderObject(m_pixelShader.get())) {
			LOG_ERROR("Failed to initialize shaders.");
			return false;
		}
		m_pipelineStateObject = renderDevice->BuildGraphicsPipelineObject(m_vertexShader.get(), m_pixelShader.get(),
			UnknownVision::GDefaultRasterizeOptions, UnknownVision::GOutputStageOptionsWithDepthTest1_1, VertexShaderType::GetVertexAttributes);
		if (!m_pipelineStateObject) {
			LOG_ERROR("Failed to create GraphicsPipelineObject.");
			return false;
		}
		{
			auto&& VSParameterDesc = VertexShaderType::GetShaderParametersStatic();
			for (size_t i = 0; i < VertexShaderType::TotalShaderParameterSlot; ++i)
			{
				m_bindingBoardForVS[i] = std::unique_ptr<UnknownVision::BindingBoard>(
					renderDevice->RequestBindingBoard(
						VSParameterDesc[i].size(), UnknownVision::DEFAULT_COMMAND_UNIT));
			}
		}
		{
			auto&& PSParameterDesc = PixelShaderType::GetShaderParametersStatic();
			for (size_t i = 0; i < PixelShaderType::TotalShaderParameterSlot; ++i)
			{
				m_bindingBoardForPS[i] = std::unique_ptr<UnknownVision::BindingBoard>(
					renderDevice->RequestBindingBoard(
						PSParameterDesc[i].size(), UnknownVision::DEFAULT_COMMAND_UNIT));
			}
		}

		return true;
	}

	bool Draw(UnknownVision::GPUResource** renderTargets, size_t renderTargetCount
		, UnknownVision::GPUResource* depthStencilBuffer
		, UnknownVision::CommandUnit* cmdUnit
		, const UnknownVision::ViewportDesc& vp, const UnknownVision::ScissorRectDesc& sr)
	{
		if (!m_pipelineStateObject || !m_bindingBoardForVS || !m_bindingBoardForPS) {
			LOG_ERROR("MeshPass is not properly initialized.");
			return false;
		}
		cmdUnit->BindPipeline(m_pipelineStateObject);
		cmdUnit->BindRenderTargets(renderTargets, renderTargetCount, depthStencilBuffer);
		cmdUnit->BindViewports(1, &vp);
		cmdUnit->BindScissorRects(1, &sr);

		// auto bind bindingboards
		const int16_t totalPipelineParameterSlots = VertexShaderType::TotalShaderParameterSlot + PixelShaderType::TotalShaderParameterSlot;
		for (int16_t slotIndex = 0; slotIndex < totalPipelineParameterSlots; ++slotIndex)
		{
			int16_t stageSlotIndex = slotIndex;
			if (stageSlotIndex < VertexShaderType::TotalShaderParameterSlot)
			{
				cmdUnit->SetBindingBoard(slotIndex, m_bindingBoardForVS[stageSlotIndex].get());
				continue;
			}
			stageSlotIndex -= VertexShaderType::TotalShaderParameterSlot;
			if (stageSlotIndex < PixelShaderType::TotalShaderParameterSlot)
			{
				cmdUnit->SetBindingBoard(slotIndex, m_bindingBoardForPS[stageSlotIndex].get());
				continue;
			}
			abort();
		}

		return true;
	}
protected:
	std::unique_ptr<VertexShaderType> m_vertexShader;
	std::unique_ptr<PixelShaderType> m_pixelShader;
	// PSO比较特殊，它由PSO Manager管理生命周期，不用我们主动销毁
	UnknownVision::GraphicsPipelineObject* m_pipelineStateObject;
	std::unique_ptr<UnknownVision::BindingBoard> m_bindingBoardForVS[VertexShaderType::TotalShaderParameterSlot];
	std::unique_ptr<UnknownVision::BindingBoard> m_bindingBoardForPS[PixelShaderType::TotalShaderParameterSlot];
};
