#include "Component.h"
#include <GraphicsInterface/RenderDevice.h>
#include "../Utility/MeshLoader/MeshLoader.h"
#include "../Utility/Image/Image.h"
#include <../Utility/InfoLog/InfoLog.h>
using namespace UnknownVision;

ResourceCenter& gRC = ResourceCenter::GetCenter();

size_t ResourceCenter::NumberOfSubresources(const std::filesystem::path& path) {
	checkInit();
	auto iter = m_pathToSubresource.find(path.generic_u8string().c_str());
	if (iter != m_pathToSubresource.end()) {
		return iter->second->resourcePtrs.size();
	}
	else {
		LOG_WARN("%s hasn't load!", path.generic_u8string().c_str());
		return SIZE_MAX;
	}
}

bool ResourceCenter::LoadMeshes(const std::filesystem::path& path)
{
	checkInit();
	std::unique_ptr<MeshLoader::IMeshLoader> loader;
	loader.reset(MeshLoader::IMeshLoader::GetLoader(MeshLoader::MeshLoaderConfigurationDesc::PNTgT0()));
	if (loader == nullptr) {
		LOG_ERROR("Create Mesh Loader Failed!");
		return false;
	}
	std::vector<MeshLoader::MeshDataContainer> container;
	if (loader->Load(path, container) == false) {
		LOG_ERROR("Load Mesh %s Failed!", path.c_str());
		return false;
	}
	std::unique_ptr<SubresourceData> subDatas(std::make_unique<SubresourceData>());
	auto cmdUnit = m_ptrDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	if (cmdUnit == nullptr) {
		LOG_ERROR("Request command unit failed!");
		return false;
	}
	for (const auto& subMesh : container) {
		SubresourceData::VectorOfResourceUnique_ptr subresource;
		{
			Buffer* idxBuffer = m_ptrDevice->CreateBuffer(subMesh.numIndices, MeshLoader::INDEX_DATA_SIZE,
				ResourceStatus(RESOURCE_USAGE_INDEX_BUFFER, RESOURCE_FLAG_STABLY));
			if (idxBuffer == nullptr) {
				LOG_ERROR("Create mesh %s index buffer failed!", path.c_str());
				return false;
			}
			if (m_ptrDevice->WriteToBuffer(const_cast<uint32_t*>(subMesh.idxBuffer.data()), idxBuffer, subMesh.numIndices* MeshLoader::INDEX_DATA_SIZE, 0, cmdUnit) == false) {
				LOG_ERROR("Write Mesh %s Index data failed!", path.c_str());
				return false;
			}
			cmdUnit->TransferState(idxBuffer, RESOURCE_STATE_INDEX_BUFFER);
			cmdUnit->Flush(true);
			subresource.push_back(std::unique_ptr<GPUResource>(idxBuffer));
		}
		{
			size_t numOfVtxBuffers = subMesh.vtxBuffers.size();
			for (size_t i = 0; i < numOfVtxBuffers; ++i) {
				Buffer* vtxBuffer = m_ptrDevice->CreateBuffer(subMesh.numVertices, subMesh.elementStrides[i],
					ResourceStatus(RESOURCE_USAGE_VERTEX_BUFFER, RESOURCE_FLAG_STABLY));
				if (vtxBuffer == nullptr) {
					LOG_ERROR("Create mesh %s vertex buffer failed!", path.c_str());
					return false;
				}
				if (m_ptrDevice->WriteToBuffer(const_cast<uint8_t*>(subMesh.vtxBuffers[i].data()), vtxBuffer, subMesh.numVertices* subMesh.elementStrides[i], 0, cmdUnit) == false) {
					LOG_ERROR("Write Mesh %s vertex buffer failed!", path.c_str());
					return false;
				}
				cmdUnit->TransferState(vtxBuffer, RESOURCE_STATE_VERTEX_BUFFER);
				cmdUnit->Flush(true);
				subresource.push_back(std::unique_ptr<GPUResource>(vtxBuffer));
			}
		}
		subDatas->resourcePtrs.emplace_back();
		subDatas->resourcePtrs.back().swap(subresource);
	}
	m_pathToSubresource[path.generic_u8string().c_str()].swap(subDatas);
	m_ptrDevice->FreeCommandUnit(&cmdUnit);
	return true;
}

bool ResourceCenter::LoadTexture2D(const std::filesystem::path& path, ElementFormatType format)
{
	checkInit();
	std::unique_ptr<MImage::Image> img = MImage::Image::LoadImageFromFile(path);
	MImage::ImageFormat desireFormat;
	switch (format) {
	case ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM:
		desireFormat = MImage::IMAGE_FORMAT_R8G8B8A8;
		break;
	case ELEMENT_FORMAT_TYPE_R8_UNORM:
		desireFormat = MImage::IMAGE_FORMAT_R8;
		break;
	default:
		LOG_ERROR("Invalid texture format!");
		return false;
	}
	img->ConvertPixelFormat(desireFormat);
	UnknownVision::ImageDesc desc;
	desc.data = img->GetData(0);
	desc.depth = 1;
	desc.width = img->Width();
	desc.height = img->Height();
	desc.rowPitch = img->GetRowPitch(0);
	desc.slicePitch = img->GetSlicePitch(0);
	std::unique_ptr<SubresourceData> subDatas(std::make_unique<SubresourceData>());
	auto texPtr = (m_ptrDevice->CreateTexture2D(desc.width, desc.height, 1, 1, format,
		ResourceStatus(RESOURCE_USAGE_SHADER_RESOURCE, RESOURCE_FLAG_STABLY)));
	auto cmdUnit = m_ptrDevice->RequestCommandUnit(DEFAULT_COMMAND_UNIT);
	if (m_ptrDevice->WriteToTexture2D({ desc }, texPtr, cmdUnit) == false) {
		LOG_ERROR("Write data to texture2D failed!");
		return false;
	}
	cmdUnit->TransferState(texPtr, RESOURCE_STATE_SHADER_RESOURCE);
	cmdUnit->Flush(true);
	subDatas->resourcePtrs.emplace_back();
	subDatas->resourcePtrs.back().push_back(std::unique_ptr<GPUResource>(texPtr));
	m_pathToSubresource[path.generic_u8string().c_str()].swap(subDatas);
	m_ptrDevice->FreeCommandUnit(&cmdUnit);
	return true;
}

void ResourceCenter::clear()
{
	m_pathToSubresource.clear();
	m_isClear = true;
}


MeshComponent* MeshComponent::Create(const std::filesystem::path& meshPath, size_t subresource)
{
	auto&& res = gRC.GetData(meshPath, subresource);
	if (res.empty() == true) {
		if (gRC.LoadMeshes(meshPath) == false) {
			LOG_ERROR("Get Mesh %s data failed!", meshPath.generic_u8string().c_str());
			abort();
		}
		res = gRC.GetData(meshPath, subresource);
	}
	if (res.empty() == true) {
		LOG_ERROR("Invalid mesh data!");
		abort();
	}
	MeshComponent* newC = new MeshComponent();
	if (res[0]->Type() != GPU_RESOURCE_TYPE_BUFFER) {
		LOG_ERROR("Invalid index buffer resource type!");
		abort();
	}
	else {
		newC->m_idxBuffer = dynamic_cast<Buffer*>(res[0]);
	}
	newC->m_vtxBuffers = decltype(newC->m_vtxBuffers)(res.size() - 1);
	for (size_t i = 1; i < res.size(); ++i) {
		if (res[i]->Type() != GPU_RESOURCE_TYPE_BUFFER) {
			LOG_ERROR("Invalid vertex buffer resource type!");
			abort();
		}
		else {
			newC->m_vtxBuffers[i - 1] = dynamic_cast<Buffer*>(res[i]);
		}
	}
	return newC;
}

PBRTextureSet* PBRTextureSet::Create(const std::filesystem::path& normalPath, const std::filesystem::path& baseColorPath, const std::filesystem::path& armPath)
{
	PBRTextureSet* newC = new PBRTextureSet();

	auto textureSetHelper = [](const std::filesystem::path& path, ElementFormatType format)->Texture2D* {
		auto&& textures = gRC.GetData(path);
		GPUResource* texture = nullptr;
		if (textures.empty()) {
			if (gRC.LoadTexture2D(path, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM) == false) {
				LOG_ERROR("Create texture failed!");
				abort();
			}
			texture = gRC.GetData(path)[0];
		}
		else {
			texture = textures[0];
		}
		if (texture == nullptr) {
			LOG_ERROR("unknown situation occured!");
			abort();
		}
		if (texture->Type() != GPU_RESOURCE_TYPE_TEXTURE2D) {
			LOG_ERROR("Invalid texture2d resource type");
			abort();
		}
		return dynamic_cast<Texture2D*>(texture);
	};

	newC->m_normal = textureSetHelper(normalPath, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM);
	newC->m_baseColor = textureSetHelper(baseColorPath, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM);
	newC->m_ao_roughness_metallic = textureSetHelper(armPath, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM);
	
	return newC;
}
