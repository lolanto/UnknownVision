#include "Viewport.h"
#include <GraphicsInterface/GPUResource.h>
#include <GraphicsInterface/CommandUnit.h>
#include <GraphicsInterface/RenderDevice.h>

bool Viewport::Init(const UnknownVision::ViewportDesc& vp, const UnknownVision::ScissorRectDesc& sr
	, UnknownVision::RenderDevice* renderDevice)
{
	m_vp = vp;
	m_sr = sr;

	m_CPUConstantBuffer.position = glm::vec4(99, 0, 99, 0); // magic number to indicate that the GPU buffer is not initialized yet
	m_CPUConstantBuffer.projMat = glm::identity<glm::mat4x4>();
	m_CPUConstantBuffer.viewMat = glm::identity<glm::mat4x4>();

	m_GPUConstantBuffer = std::unique_ptr<UnknownVision::GPUBuffer>(renderDevice->CreateBuffer<ShaderConstantBuffer>(
		UnknownVision::ResourceStatus(UnknownVision::RESOURCE_USAGE_CONSTANT_BUFFER, UnknownVision::RESOURCE_FLAG_FREQUENTLY)));

	return true;
}

bool Viewport::Update(const Viewport::ShaderConstantBuffer& newData
	, UnknownVision::CommandUnit* cmdUnit
	, UnknownVision::RenderDevice* renderDevice)
{
	if (!m_GPUConstantBuffer || !cmdUnit) {
		return false;
	}
	bool isDataChanged = (m_CPUConstantBuffer != newData);
	if (isDataChanged)
	{
		cmdUnit->TransferState(m_GPUConstantBuffer.get(), UnknownVision::RESOURCE_STATE_COPY_DEST);
		m_CPUConstantBuffer = newData;
		renderDevice->WriteToBuffer(&m_CPUConstantBuffer, m_GPUConstantBuffer.get(), sizeof(ShaderConstantBuffer), 0, cmdUnit);
		cmdUnit->TransferState(m_GPUConstantBuffer.get(), UnknownVision::RESOURCE_STATE_CONSTANT_BUFFER);
		cmdUnit->Flush(true);
	}
	return true;
}
