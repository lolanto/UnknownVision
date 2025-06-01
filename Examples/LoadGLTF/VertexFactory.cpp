#include "VertexFactory.h"
#include "IMeshResource.h"
#include <GraphicsInterface/CommandUnit.h>
#include <GraphicsInterface/RenderDevice.h>
#include <GraphicsInterface/GPUResource.h>

BEG_NAME_SPACE

const std::vector<VertexAttribute> VertexFactory_PNT0::GetVertexAttributes()
{
	return {
		{VERTEX_ATTRIBUTE_TYPE_POSITION, ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0, 0, 0 }, // Position
		{VERTEX_ATTRIBUTE_TYPE_TEXTURE, ELEMENT_FORMAT_TYPE_R32G32_FLOAT, 0, 1, 0 }, // Texcoord
		{VERTEX_ATTRIBUTE_TYPE_NORMAL, ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0, 2, 0 } // Normal
	};
}

VertexBuffersHolder VertexFactory_PNT0::CreateVertexBuffersHolder(const IMeshResource* inputMesh, CommandUnit* cmdUnit, RenderDevice* device)
{
	// 准备 Vertex Buffer position
	std::unique_ptr<GPUBuffer> vtxBuffer_pos(
		device->CreateBuffer(
			inputMesh->GetVertexCount()
			, inputMesh->GetVertexElementByteSize(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_POSITION, 0)
			, ResourceStatus(ResourceUsages::RESOURCE_USAGE_VERTEX_BUFFER, ResourceFlags::RESOURCE_FLAG_STABLY)));
	{
		device->WriteToBuffer(
			inputMesh->GetVertexData(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_POSITION, 0)
			, vtxBuffer_pos.get()
			, inputMesh->GetVertexDataByteSize(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_POSITION, 0)
			, 0
			, cmdUnit);
		cmdUnit->TransferState(vtxBuffer_pos.get(), RESOURCE_STATE_VERTEX_BUFFER);
		cmdUnit->Flush(true);
	}
	// 准备 Vertex Buffer texture coordinate
	std::unique_ptr<GPUBuffer> vtxBuffer_tex(
		device ->CreateBuffer(
			inputMesh->GetVertexCount()
			, inputMesh->GetVertexElementByteSize(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_TEXTURE, 0)
			, ResourceStatus(ResourceUsages::RESOURCE_USAGE_VERTEX_BUFFER, ResourceFlags::RESOURCE_FLAG_STABLY)));
	{
		device->WriteToBuffer(
			inputMesh->GetVertexData(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_TEXTURE, 0)
			, vtxBuffer_tex.get()
			, inputMesh->GetVertexDataByteSize(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_TEXTURE, 0)
			, 0
			, cmdUnit);
		cmdUnit->TransferState(vtxBuffer_tex.get(), RESOURCE_STATE_VERTEX_BUFFER);
		cmdUnit->Flush(true);
	}
	// 准备 Vertex Buffer normal
	std::unique_ptr<GPUBuffer> vtxBuffer_nor(
		device->CreateBuffer(
			inputMesh->GetVertexCount()
			, inputMesh->GetVertexElementByteSize(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_NORMAL, 0)
			, ResourceStatus(ResourceUsages::RESOURCE_USAGE_VERTEX_BUFFER, ResourceFlags::RESOURCE_FLAG_STABLY)));
	{
		device->WriteToBuffer(
			inputMesh->GetVertexData(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_NORMAL, 0)
			, vtxBuffer_nor.get()
			, inputMesh->GetVertexDataByteSize(VertexAttributeType::VERTEX_ATTRIBUTE_TYPE_NORMAL, 0)
			, 0
			, cmdUnit);
		cmdUnit->TransferState(vtxBuffer_tex.get(), RESOURCE_STATE_VERTEX_BUFFER);
		cmdUnit->Flush(true);
	}

	// 准备 Index Buffer
	std::unique_ptr<GPUBuffer> idxBuffer(
		device->CreateBuffer(
			inputMesh->GetIndexCount()
			, inputMesh->GetIndexElementByteSize()
			, ResourceStatus(ResourceUsages::RESOURCE_USAGE_INDEX_BUFFER, ResourceFlags::RESOURCE_FLAG_STABLY)));
	{
		device->WriteToBuffer(
			inputMesh->GetIndexData()
			, idxBuffer.get()
			, inputMesh->GetIndexDataByteSize()
			, 0
			, cmdUnit);
		cmdUnit->TransferState(idxBuffer.get(), RESOURCE_STATE_INDEX_BUFFER);
		cmdUnit->Flush(true);
	}

	VertexBuffersHolder holder;
	holder.VertexBuffers.push_back(std::move(vtxBuffer_pos));
	holder.VertexBuffers.push_back(std::move(vtxBuffer_tex));
	holder.VertexBuffers.push_back(std::move(vtxBuffer_nor));
	holder.IndexBuffer = std::move(idxBuffer);
	holder.BindingFunction = &VertexFactory_PNT0::BindingVertexBuffers;

	holder.VertexCount = inputMesh->GetVertexCount();
	holder.IndexCount = inputMesh->GetIndexCount();
	return holder;
}

void VertexFactory_PNT0::BindingVertexBuffers(VertexBuffersHolder* holder, CommandUnit* cmdUnit)
{
	GPUBuffer* vertexBuffers[3];
	for (size_t i = 0; i < holder->VertexBuffers.size(); ++i)
	{
		vertexBuffers[i] = holder->VertexBuffers[i].get();
	}
	cmdUnit->BindVertexBuffers(0, 3, &vertexBuffers[0]);
	cmdUnit->BindIndexBuffer(holder->IndexBuffer.get());
}

END_NAME_SPACE
