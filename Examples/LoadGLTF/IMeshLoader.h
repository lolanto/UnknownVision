#pragma once
#include <UVConfig.h>
#include <filesystem>
#include <memory>

BEG_NAME_SPACE
class IMeshResource;

/** 对网格类型资源加载过程进行抽象，底下可能是不同类型的加载器实现 */
class IMeshLoader : public Standalone {
public:
	virtual ~IMeshLoader() = default;
protected:
	IMeshLoader() = default;

public:
	/** 从内存加载网格资源
	 * @param data 内存数据
	 * @param bytesOfData 内存数据大小
	 * @return 返回加载的网格资源对象，无法加载或者加载失败返回nullptr
	 */
	virtual std::unique_ptr<IMeshResource> LoadMeshFromMemory(const void* data, size_t bytesOfData) { return nullptr; }

	/** 从文件系统加载网格资源
	 * @param fileName 文件名
	 * @param subMeshIndex 子网格索引
	 * @param subModelIndex 子模型索引
	 * @return 返回加载的网格资源对象，无法加载或者加载失败返回nullptr */
	virtual std::unique_ptr<IMeshResource> LoadMeshFromFile(const std::filesystem::path& path, uint8_t subMeshIndex = 0, uint8_t subModelIndex = 0) { return nullptr; }

	/** 获取加载器名称 */
	virtual const char* GetLoaderName() const { return "Unknown Mesh Loader"; }
};

END_NAME_SPACE
