#include "MeshLoader.h"
#include <../Utility/InfoLog/InfoLog.h>
#include <../Utility/MathInterface/MathInterface.hpp>
#ifdef USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace MeshLoader;
using IMath::IFLOAT3;
using IMath::IFLOAT2;

static_assert(sizeof(IMath::IFLOAT3) == NORMAL_DATA_SIZE);
static_assert(sizeof(IMath::IFLOAT3) == POSITION_DATA_SIZE);
static_assert(sizeof(IMath::IFLOAT3) == TANGENT_DATA_SIZE);
static_assert(sizeof(IMath::IFLOAT3) == BITANGENT_DATA_SIZE);
static_assert(sizeof(IMath::IFLOAT2) == TEXCOORD_DATA_SIZE);


MeshLoaderConfigurationDesc MeshLoaderConfigurationDesc::P() {
	MeshLoaderConfigurationDesc config;
	config.bufferSettings = { { VERTEX_DATA_TYPE_POSITION } };
	config.globalSettings = VERTEX_DATA_TYPE_POSITION;
	return config;
}

MeshLoaderConfigurationDesc MeshLoaderConfigurationDesc::PT0() {
	MeshLoaderConfigurationDesc config;
	config.bufferSettings = { { VERTEX_DATA_TYPE_POSITION, VERTEX_DATA_TYPE_TEXCOORD0 } };
	config.globalSettings = VERTEX_DATA_TYPE_POSITION | VERTEX_DATA_TYPE_TEXCOORD0;
	return config;
}

MeshLoaderConfigurationDesc MeshLoaderConfigurationDesc::PNT0() {
	MeshLoaderConfigurationDesc config;
	config.bufferSettings = { { VERTEX_DATA_TYPE_POSITION, VERTEX_DATA_TYPE_NORMAL, VERTEX_DATA_TYPE_TEXCOORD0 } };
	config.globalSettings = VERTEX_DATA_TYPE_POSITION | VERTEX_DATA_TYPE_NORMAL | VERTEX_DATA_TYPE_TEXCOORD0;
	return config;
}

MeshLoaderConfigurationDesc MeshLoader::MeshLoaderConfigurationDesc::PN()
{
	MeshLoaderConfigurationDesc config;
	config.bufferSettings = { { VERTEX_DATA_TYPE_POSITION, VERTEX_DATA_TYPE_NORMAL } };
	config.globalSettings = VERTEX_DATA_TYPE_POSITION | VERTEX_DATA_TYPE_NORMAL;
	return config;
}

MeshLoaderConfigurationDesc MeshLoader::MeshLoaderConfigurationDesc::PNTgT0()
{
	MeshLoaderConfigurationDesc config;
	config.bufferSettings = { { VERTEX_DATA_TYPE_POSITION, VERTEX_DATA_TYPE_NORMAL, VERTEX_DATA_TYPE_TANGENT, VERTEX_DATA_TYPE_TEXCOORD0 } };
	config.globalSettings = VERTEX_DATA_TYPE_POSITION | VERTEX_DATA_TYPE_NORMAL | VERTEX_DATA_TYPE_TANGENT | VERTEX_DATA_TYPE_TEXCOORD0;
	return config;
}

class AssimpMeshLoader : public IMeshLoader{
public:
	AssimpMeshLoader(const MeshLoaderConfigurationDesc& config) : IMeshLoader(config) {}
	virtual ~AssimpMeshLoader() = default;
	virtual bool Load(std::filesystem::path path, std::vector<MeshDataContainer>& output) override final {
		int step = aiProcess_Triangulate
#ifdef FLIP_UV
			| aiProcess_FlipUVs
#endif // FLIP_UV
			;
		if (m_config.globalSettings & VERTEX_DATA_TYPE_NORMAL) step |= aiProcess_GenNormals;
		if (m_config.globalSettings & VERTEX_DATA_TYPE_BITANGENT || m_config.globalSettings & VERTEX_DATA_TYPE_TANGENT) step |= aiProcess_CalcTangentSpace;
		const aiScene* scene = m_importer.ReadFile(path.generic_u8string(), step);
		if (scene == nullptr || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr) {
			LOG_WARN("Read model file %s failed!", path.generic_string().c_str());
			LOG_WARN("Error message from assimp: %s", m_importer.GetErrorString());
			return false;
		}
		if (processNode(scene->mRootNode, scene, output) == false) {
			output.clear();
			LOG_WARN("Load model from file %s failed!", path.generic_string().c_str());
			return false;
		}
		m_importer.FreeScene();
		return true;
	}
private:
	bool processNode(aiNode* node, const aiScene* scene, std::vector<MeshDataContainer>& output) {
		for (size_t mesh_idx = 0; mesh_idx < node->mNumMeshes; ++mesh_idx) {
			aiMesh* meshPtr = scene->mMeshes[node->mMeshes[mesh_idx]];
			MeshDataContainer container;
			if (processMesh(meshPtr, container) == false) {
				LOG_WARN("Load Mesh %s Failed!", meshPtr->mName.C_Str());
				return false;
			}
			output.push_back(std::move(container));
		}
		for (size_t node_idx = 0; node_idx < node->mNumChildren; ++node_idx) {
			if (processNode(node->mChildren[node_idx], scene, output) == false) {
				return false;
			}
		}
		return true;
	}
	bool processMesh(aiMesh* meshPtr, MeshDataContainer& container) {
		container.vtxBuffers.clear();
		container.vtxBuffers.resize(m_config.bufferSettings.size());
		container.elementStrides.resize(m_config.bufferSettings.size());
		for (size_t bf_idx = 0; bf_idx < m_config.bufferSettings.size(); ++bf_idx) {
			std::vector<uint8_t>& targetBuffer = container.vtxBuffers[bf_idx];
			/** 预计算一个顶点的数据大小 */
			size_t sizeOfVertexData = 0;
			for (int prop_idx = 0; prop_idx < m_config.bufferSettings[bf_idx].size(); ++prop_idx) {
				switch (m_config.bufferSettings[bf_idx][prop_idx]) {
				case VERTEX_DATA_TYPE_POSITION: 
					sizeOfVertexData += POSITION_DATA_SIZE; 
					break;
				case VERTEX_DATA_TYPE_NORMAL: 
					sizeOfVertexData += NORMAL_DATA_SIZE;
					if (meshPtr->mNormals == nullptr) {
						LOG_WARN("mesh %s doesn't have normal data!", meshPtr->mName.C_Str());
						return false;
					}
					break;
				case VERTEX_DATA_TYPE_TANGENT: 
					sizeOfVertexData += TANGENT_DATA_SIZE;
					if (meshPtr->mTangents == nullptr) {
						LOG_WARN("mesh %s doesn't have tangent data!", meshPtr->mName.C_Str());
						return false;
					}
					break;
				case VERTEX_DATA_TYPE_BITANGENT: 
					sizeOfVertexData += BITANGENT_DATA_SIZE; 
					if (meshPtr->mBitangents == nullptr) {
						LOG_WARN("mesh %s doesn't have bitangent data!", meshPtr->mName.C_Str());
						return false;
					}
					break;
				case VERTEX_DATA_TYPE_TEXCOORD0: 
					sizeOfVertexData += TEXCOORD_DATA_SIZE;
					if (meshPtr->mTextureCoords == nullptr) {
						LOG_WARN("mesh %s doesn't have uv data!", meshPtr->mName.C_Str());
						return false;
					}
					break;
				default:
					LOG_WARN("Unreconized vertex data type!");
					return false;
				}
			}
			container.elementStrides[bf_idx] = sizeOfVertexData;
			/** 处理顶点数据 */
			targetBuffer.clear();
			targetBuffer.resize(sizeOfVertexData * meshPtr->mNumVertices);
			size_t bufferOffset = 0;
			for (size_t vtx_idx = 0; vtx_idx < meshPtr->mNumVertices; ++vtx_idx) {
				for (int prop_idx = 0; prop_idx < m_config.bufferSettings[bf_idx].size(); ++prop_idx) {
					IFLOAT3 f3;
					IFLOAT2 f2;
					switch (m_config.bufferSettings[bf_idx][prop_idx]) {
					case VERTEX_DATA_TYPE_POSITION:
						f3 = { meshPtr->mVertices[vtx_idx].x, meshPtr->mVertices[vtx_idx].y, meshPtr->mVertices[vtx_idx].z };
						memcpy(&targetBuffer[bufferOffset], &f3, POSITION_DATA_SIZE);
						bufferOffset += POSITION_DATA_SIZE;
						break;
					case VERTEX_DATA_TYPE_NORMAL:
						f3 = { meshPtr->mNormals[vtx_idx].x, meshPtr->mNormals[vtx_idx].y, meshPtr->mNormals[vtx_idx].z };
						memcpy(&targetBuffer[bufferOffset], &f3, NORMAL_DATA_SIZE);
						bufferOffset += NORMAL_DATA_SIZE;
						break;
					case VERTEX_DATA_TYPE_TANGENT:
						f3 = { meshPtr->mTangents[vtx_idx].x, meshPtr->mTangents[vtx_idx].y, meshPtr->mTangents[vtx_idx].z };
						memcpy(&targetBuffer[bufferOffset], &f3, TANGENT_DATA_SIZE);
						bufferOffset += TANGENT_DATA_SIZE;
						break;
					case VERTEX_DATA_TYPE_BITANGENT:
						f3 = { meshPtr->mBitangents[vtx_idx].x, meshPtr->mBitangents[vtx_idx].y, meshPtr->mBitangents[vtx_idx].z };
						memcpy(&targetBuffer[bufferOffset], &f3, BITANGENT_DATA_SIZE);
						bufferOffset += BITANGENT_DATA_SIZE;
						break;
					case VERTEX_DATA_TYPE_TEXCOORD0:
						f2 = { meshPtr->mTextureCoords[0][vtx_idx].x, meshPtr->mTextureCoords[0][vtx_idx].y };
						memcpy(&targetBuffer[bufferOffset], &f2, TEXCOORD_DATA_SIZE);
						bufferOffset += TEXCOORD_DATA_SIZE;
						break;
					default:
						LOG_WARN("Unreconized vertex data type!");
						return false;
					}
				}
			}
		}
		/** 计算索引数据 */
		container.idxBuffer.clear();
		container.idxBuffer.reserve(static_cast<size_t>(meshPtr->mNumFaces) * 3); /**< Note: 假设每一个面都是三角面! */
		for (size_t face_idx = 0; face_idx < meshPtr->mNumFaces; ++face_idx) {
			aiFace face = meshPtr->mFaces[face_idx];
			for (int idx = 0; idx < face.mNumIndices; ++idx) {
				container.idxBuffer.push_back(face.mIndices[idx]);
			}
		}
		container.numVertices = meshPtr->mNumVertices;
		container.numIndices = container.idxBuffer.size();
		return true;
	}
private:
	Assimp::Importer m_importer;
};

IMeshLoader* IMeshLoader::GetLoader(const MeshLoaderConfigurationDesc& config) {
	return new AssimpMeshLoader(config);
}

#endif // USE_ASSIMP