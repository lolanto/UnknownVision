#pragma once

#include <UVConfig.h>
#include "IMeshLoader.h"
#include "IMeshResource.h"
#include <InfoLog/InfoLog.h>
#include <MathInterface/MathInterface.hpp>

#include <vector>
#include <string>

BEG_NAME_SPACE

/** GLTF类型的网格加载器以及网格资源实现 */
class MeshLoaderGLTF;

/** GLTF类型的网格资源实现
 * 该实现仅支持GLTF格式的网格资源数据 */
class MeshResourceGLTF final : public IMeshResource {
	friend class MeshLoaderGLTF;
public:
	MeshResourceGLTF() = default;
	MeshResourceGLTF(MeshResourceGLTF&& rhs) = default;
	MeshResourceGLTF(const MeshResourceGLTF& rhs) = default;
	virtual ~MeshResourceGLTF() = default;

public:
	virtual const void* GetVertexData(VertexAttributeType attr, uint8_t index) const override;
	virtual const void* GetIndexData() const override;
	virtual size_t GetVertexCount() const override;
	virtual size_t GetIndexCount() const override;
	virtual ElementFormatType GetVertexFormat(VertexAttributeType attr, uint8_t index) const override;
	virtual ElementFormatType GetIndexFormat() const override;
	virtual const char* GetMeshName() override { return m_name.c_str(); }
private:
	/** 检查网格资源的合法性，在必要的函数调用中开启检查 */
	bool validate() const;
	std::string m_name;
	std::vector<glm::f32vec3> m_vtxBufferPosition;
	std::vector<glm::f32vec3> m_vtxBufferNormal;
	std::vector<glm::f32vec2> m_vtxBufferTexcoord;
	std::vector<glm::uint32_t> m_idxBuffer_32;
	std::vector<glm::uint16_t> m_idxBuffer_16;
	ElementFormatType m_idxFormatType;
	uint32_t m_idxCount;
};

/** GLTF类型的网格加载器实现
 * 该实现仅支持GLTF格式的网格资源数据 */
class MeshLoaderGLTF final : public IMeshLoader {
public:
	static MeshLoaderGLTF& GetInstance() {
		static MeshLoaderGLTF _inst;
		return _inst;
	}
public:
	virtual ~MeshLoaderGLTF() = default;

public:
	virtual std::unique_ptr<IMeshResource> LoadMeshFromFile(const std::filesystem::path& path, uint8_t subModelIndex = 0, uint8_t subMeshIndex = 0) override;
	virtual const char* GetLoaderName() const { return "GLTF Mesh Loader"; }
};

END_NAME_SPACE
