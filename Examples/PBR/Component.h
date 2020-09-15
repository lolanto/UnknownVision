#pragma once
#include <GraphicsInterface/GPUResource.h>
#include <../Utility/MathInterface/MathInterface.hpp>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>



/** 临时资源中心，单例，已加载资源存储访问，没有状态管理! */
class ResourceCenter {
public:
	struct SubresourceData {
		using VectorOfResourceUnique_ptr = std::vector<std::unique_ptr<UnknownVision::GPUResource>>;
		using VectorOfResourcePtr = std::vector<UnknownVision::GPUResource*>;
		std::vector<std::vector<std::unique_ptr<UnknownVision::GPUResource>>> resourcePtrs;
		SubresourceData() : resourcePtrs() {};
		SubresourceData(SubresourceData&& rhs) {
			resourcePtrs.swap(rhs.resourcePtrs);
		}
		SubresourceData(const SubresourceData& rhs) = delete;
		const SubresourceData& operator=(SubresourceData&& rhs) {
			resourcePtrs.swap(rhs.resourcePtrs);
		}
		const SubresourceData& operator=(const SubresourceData& rhs) = delete;
	};
private:
	ResourceCenter() : m_ptrDevice(nullptr), m_isInit(false), m_isClear(true) {}
public:
	~ResourceCenter() { if (m_isClear == false) abort(); }
	static ResourceCenter& GetCenter() {
		static ResourceCenter _instance;
		return _instance;
	}
public:
	/** 在调用任何ResourceCenter相关函数前调用 */
	void Init(UnknownVision::RenderDevice* ptrDev) { m_ptrDevice = ptrDev; m_isInit = true; }
	/** 务必在Device被删除前调用 */
	void Release() { clear(); }
	SubresourceData::VectorOfResourcePtr GetData(const std::filesystem::path& path, int subresource = 0) {
		checkInit();
		auto iter = m_pathToSubresource.find(path.generic_u8string().c_str());
		if (iter != m_pathToSubresource.end()) {
			auto& vec = iter->second->resourcePtrs[subresource];
			SubresourceData::VectorOfResourcePtr res(vec.size(), nullptr);
			for (size_t i = 0; i < vec.size(); ++i) res[i] = vec[i].get();
			return res;
		}
		else return {};
	}
	size_t NumberOfSubresources(const std::filesystem::path& path);
	bool LoadMeshes(const std::filesystem::path& path);
	bool LoadTexture2D(const std::filesystem::path& path, UnknownVision::ElementFormatType format);
private:
	void clear();
	void checkInit() { if (m_isInit == false) abort(); /* Haven't init! */ }
private:
	std::unordered_map<std::string, std::unique_ptr<SubresourceData>> m_pathToSubresource;
	UnknownVision::RenderDevice* m_ptrDevice;
	bool m_isInit;
	bool m_isClear;
};

class IComponent {
public:
	// static constexpr size_t ID = 0; Note: 继承者必须拥有该静态成员
public:
	virtual bool Valid() const { return false; }
public:
	virtual ~IComponent() = default;
};

#ifndef COMPONENT_ID
#define COMPONENT_ID(x) static constexpr size_t ID = x
#endif // COMPONENT_ID

class MeshComponent : public IComponent {
public:
	static MeshComponent* Create(const std::filesystem::path& meshPath, size_t subresource = 0);
	MeshComponent() : m_idxBuffer(nullptr), m_vtxBuffers() {}
	virtual ~MeshComponent() = default;
public:
	COMPONENT_ID(0);
	virtual bool Valid() const override final { return m_idxBuffer != nullptr && m_vtxBuffers.empty() == false; }
	UnknownVision::Buffer* IndexBuffer() const { return m_idxBuffer; }
	std::vector<UnknownVision::Buffer*> VertexBuffers() const { return m_vtxBuffers; }
private:
	UnknownVision::Buffer* m_idxBuffer;
	std::vector<UnknownVision::Buffer*> m_vtxBuffers;
};

class PBRTextureSet : public IComponent {
public:
	static PBRTextureSet* Create(
		const std::filesystem::path& normalPath,
		const std::filesystem::path& baseColorPath,
		const std::filesystem::path& armPath);
	PBRTextureSet() : m_normal(nullptr), m_baseColor(nullptr), m_ao_roughness_metallic(nullptr) {}
	virtual ~PBRTextureSet() = default;
public:
	COMPONENT_ID(1);
	virtual bool Valid() const override final { return m_normal != nullptr && m_baseColor != nullptr && m_ao_roughness_metallic != nullptr; }
	UnknownVision::Texture2D* NormalTexture() const { return m_normal; }
	UnknownVision::Texture2D* BaseColorTexture() const { return m_baseColor; }
	UnknownVision::Texture2D* AO_Roughness_Metallic() const { return m_ao_roughness_metallic; }
private:
	UnknownVision::Texture2D* m_normal;
	UnknownVision::Texture2D* m_baseColor;
	UnknownVision::Texture2D* m_ao_roughness_metallic;
};

class ITransformComponent : public IComponent {
public:
	static ITransformComponent* Create(IMath::IFLOAT3 position);
	ITransformComponent() : m_modelMatrix(nullptr) {}
	virtual ~ITransformComponent() = default;
public:
	COMPONENT_ID(2);
	virtual bool Valid() const override final { return m_modelMatrix != nullptr; }
public:
	virtual void Translate(IMath::IFLOAT3 offset) = 0;
	virtual void RotateAround(float radiance, IMath::IFLOAT3 axis) = 0;
	virtual UnknownVision::Buffer* ModelMatrix(UnknownVision::RenderDevice*) { return m_modelMatrix.get(); }
protected:
	std::unique_ptr < UnknownVision:: Buffer > m_modelMatrix;
};


