#include "MeshImpl_GLTF.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tinygltf/tiny_gltf.h>

BEG_NAME_SPACE

bool MeshResourceGLTF::validate() const
{
	bool valid = true;
#ifdef _DEBUG
	valid &= m_vtxBufferPosition.size() > 0;
	// 网格顶点数据一旦存在(非空)，则大小必须与位置数据一致
	if (m_vtxBufferNormal.size())
		valid &= m_vtxBufferNormal.size() == m_vtxBufferPosition.size();
	if (m_vtxBufferTexcoord.size())
		valid &= m_vtxBufferTexcoord.size() == m_vtxBufferPosition.size();
#endif // _DEBUG
	return valid;
}

const void* MeshResourceGLTF::GetVertexData(VertexAttributeType attr, uint8_t index) const
{
	switch (attr)
	{
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION:
		return m_vtxBufferPosition.data();
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_NORMAL:
		return m_vtxBufferNormal.data();
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_TEXTURE:
		if (index == 0)
			return m_vtxBufferTexcoord.data();
		else
			return nullptr;
	default:
		return nullptr;
	}
}

const void* MeshResourceGLTF::GetIndexData() const
{
	if (m_idxFormatType == ELEMENT_FORMAT_TYPE_R32_UINT)
	{
		return m_idxBuffer_32.data();
	}
	else if (m_idxFormatType == ELEMENT_FORMAT_TYPE_R16_UINT)
	{
		return m_idxBuffer_16.data();
	}
	else
	{
		LOG_ERROR("Invalid index format type: %d", m_idxFormatType);
		return nullptr;
	}
}

size_t MeshResourceGLTF::GetVertexCount() const
{
	validate();
	return m_vtxBufferPosition.size();
}

size_t MeshResourceGLTF::GetIndexCount() const
{
	return m_idxCount;
}

ElementFormatType MeshResourceGLTF::GetVertexFormat(VertexAttributeType attr, uint8_t index) const
{
	switch (attr)
	{
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION:
		return ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT;
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_NORMAL:
		return ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT;
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_TEXTURE:
		if (index == 0)
			return ELEMENT_FORMAT_TYPE_R32G32_FLOAT;
		else
			return ELEMENT_FORMAT_TYPE_INVALID;
	default:
		return ELEMENT_FORMAT_TYPE_INVALID;
	}
}

ElementFormatType MeshResourceGLTF::GetIndexFormat() const
{
	return m_idxFormatType;
}


size_t ByteSizeOfComponentType(int componentType)
{
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		return sizeof(int8_t);
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		return sizeof(uint8_t);
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		return sizeof(int16_t);
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return sizeof(uint16_t);
	case TINYGLTF_COMPONENT_TYPE_INT:
		return sizeof(int32_t);
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return sizeof(uint32_t);
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		return sizeof(float);
	case TINYGLTF_COMPONENT_TYPE_DOUBLE:
		return sizeof(double);
	default:
		LOG_ERROR("Unknown component type: %d", componentType);
		abort();
		return 0;
	}
}

size_t ElementCountOfType(int type)
{
	switch (type)
	{
	case TINYGLTF_TYPE_SCALAR:
		return 1; // 单个标量
	case TINYGLTF_TYPE_VEC2:
			return 2;
	case TINYGLTF_TYPE_VEC3:
		return 3;
	case TINYGLTF_TYPE_VEC4:
		return 4;
	default:
		LOG_ERROR("Unknown type: %d", type);
		abort();
		return 0;
	}
}

/**
 * @brief 负责解析GLTF模型文件中的Buffer数据
 * @param model 加载的GLTF模型文件对象
 * @param primitive 当前正在处理submesh gltf对象
 * @param accessorIndex Buffer访问器的下标
 * @param gltfPropertyType GLTF属性类型(e.g. 常量，向量)
 * @param gltfPropertyComponentType GLTF属性组件类型(e.g. float, int)
 * @param buffer 要填充的目标缓冲区
 * @return 是否成功加载
 */
template<typename PropertyType, typename ElementCountType = void>
bool GLTFBufferViewerHelperFunc(const tinygltf::Model& model, const tinygltf::Primitive& primitive
	, size_t accessorIndex
	, int gltfPropertyType, int gltfPropertyComponentType
	, std::vector<PropertyType>& outBuffer)
{
	const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
	if (accessor.type != gltfPropertyType || accessor.componentType != gltfPropertyComponentType)
	{
		LOG_ERROR("attribute type mismatch");
		return false;
	}
	
	size_t numElements = accessor.count;
	const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& bufferData = model.buffers[bufferView.buffer];
	const uint8_t* bufferPtr = bufferData.data.data() + bufferView.byteOffset + accessor.byteOffset;
	size_t byteStride = bufferView.byteStride;
	if (byteStride == 0)
		byteStride = sizeof(PropertyType);

	outBuffer.resize(numElements);

	for (size_t i = 0; i < numElements; ++i) {
		memcpy_s(&outBuffer[i], sizeof(PropertyType), bufferPtr + i * byteStride, sizeof(PropertyType));
	}

	return true;
}

/** 
 * @brief 负责将GLTF模型中的属性数据加载到缓冲区中
 * @param model 加载的GLTF模型文件对象
 * @param primitive 当前正在处理submesh gltf对象
 * @param propertyName 属性名称(e.g. POSITION, TEXCOORD_0)
 * @param isImportance 这个属性是否是必须的
 * @param gltfPropertyType GLTF属性类型(e.g. 常量，向量)
 * @param gltfPropertyComponentType GLTF属性组件类型(e.g. float, int)
 * @param buffer 要填充的目标缓冲区
 * @return 是否成功加载
 */
template<typename PropertyType>
bool GLTFPropertyViewerHelperFunc(const tinygltf::Model& model, const tinygltf::Primitive& primitive
	, const std::string& propertyName, bool isImportance
	, int gltfPropertyType, int gltfPropertyComponentType
	, std::vector<PropertyType>& outBuffer) 
{
	auto propertyIter = primitive.attributes.find(propertyName);
	bool hasAttribute = (propertyIter != primitive.attributes.end());
	if (hasAttribute) {
		size_t accessorIndex = propertyIter->second;
		return GLTFBufferViewerHelperFunc(model, primitive, accessorIndex, gltfPropertyType, gltfPropertyComponentType, outBuffer);
	}
	else if (isImportance) {
		LOG_ERROR("does not have %s attribute", propertyName.c_str());
		return false;
	}
	return true;
}

std::unique_ptr<IMeshResource> MeshLoaderGLTF::LoadMeshFromFile(const std::filesystem::path& path, uint8_t subModelIndex, uint8_t subMeshIndex)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	bool loadResult = false;
	if (path.extension().compare(".gltf") == 0)
		loadResult = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
	else if (path.extension().compare(".glb") == 0)
		loadResult = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
	
	if (!warn.empty()) {
		LOG_WARN("Warn: %s", warn.c_str());
	}
	if (!err.empty()) {
		LOG_ERROR("Error: %s", err.c_str());
	}
	if (!loadResult) {
		LOG_ERROR("Failed to load file: %s", path.string().c_str());
		return nullptr;
	}

	std::unique_ptr<MeshResourceGLTF> mesh = std::make_unique<MeshResourceGLTF>();
	mesh->m_name = path.stem().string();
	bool hasSpecificMesh = true;
	if (subModelIndex >= model.meshes.size()) {
		LOG_ERROR("SubModelIndex %d is out of range, total %d", subModelIndex, model.meshes.size());
		hasSpecificMesh = false;
	}
	if (subMeshIndex >= model.meshes[subModelIndex].primitives.size()) {
		LOG_ERROR("SubMeshIndex %d is out of range, total %d", subMeshIndex, model.meshes[subModelIndex].primitives.size());
		hasSpecificMesh = false;
	}
	if (!hasSpecificMesh) {
		LOG_ERROR("Failed to load mesh %s, subModelIndex %d, subMeshIndex %d", path.string().c_str(), subModelIndex, subMeshIndex);
		return nullptr;
	}
	tinygltf::Primitive& primitive = model.meshes[subModelIndex].primitives[subMeshIndex];

	// Index
	bool hasGotIndexBuffer = false;
	if (GLTFBufferViewerHelperFunc(model, primitive, primitive.indices, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, mesh->m_idxBuffer_16))
	{
		hasGotIndexBuffer = true;
		mesh->m_idxCount = mesh->m_idxBuffer_16.size();
		mesh->m_idxFormatType = ELEMENT_FORMAT_TYPE_R16_UINT;
	}
	else if (GLTFBufferViewerHelperFunc(model, primitive, primitive.indices, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, mesh->m_idxBuffer_32))
	{
		hasGotIndexBuffer = true;
		mesh->m_idxCount = mesh->m_idxBuffer_32.size();
		mesh->m_idxFormatType = ELEMENT_FORMAT_TYPE_R32_UINT;
	}
	if (!hasGotIndexBuffer)
	{
		LOG_ERROR("Failed to load index data of Mesh %s", path.string().c_str());
		return nullptr;
	}
	// Position
	if (!GLTFPropertyViewerHelperFunc(model, primitive, "POSITION", true, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT, mesh->m_vtxBufferPosition))
	{
		LOG_ERROR("Failed to load position data of Mesh %s", path.string().c_str());
		return nullptr;
	}
	// Normal
	if (!GLTFPropertyViewerHelperFunc(model, primitive, "NORMAL", false, TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT, mesh->m_vtxBufferNormal))
	{
		LOG_WARN("Failed to load normal data of Mesh %s", path.string().c_str());
	}
	// Texcoord
	if (!GLTFPropertyViewerHelperFunc(model, primitive, "TEXCOORD_0", false, TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT, mesh->m_vtxBufferTexcoord))
	{
		LOG_WARN("Failed to load texcoord data of Mesh %s", path.string().c_str());
	}

	return mesh;
}

END_NAME_SPACE
