#pragma once
#include <UVConfig.h>
#include <cstdint>

BEG_NAME_SPACE

class IMeshLoader;

/** 对网格资源对象的抽象
 * 负责存储诸如顶点数据，索引数据等
 * 以及必要的信息查询接口 */
class IMeshResource {
public:
	IMeshResource() = default;
	virtual ~IMeshResource() = default;

	/** 获取顶点数据
	 * @param attr 要访问的顶点属性类型
	 * @param index 顶点属性类型的下标，通常是0，除非是类似texcoord1这种，就需要填1
	 * @return 返回nullptr，说明该属性不存在
	 */
	virtual const void* GetVertexData(VertexAttributeType attr, uint8_t index) const = 0;

	/** 获取索引数据 
	 * @return 返回nullptr, 说明该属性不存在*/
	virtual const void* GetIndexData() const = 0;

	/** 获取顶点数量 */
	virtual size_t GetVertexCount() const = 0;

	/** 获取索引数量 */ 
	virtual size_t GetIndexCount() const = 0;

	/** 获取顶点数据的格式
	 * @param attr 要访问的顶点属性类型
	 * @param index 顶点属性类型的下标，通常是0，除非是类似texcoord1这种，就需要填1
	 * @return 返回ELEMENT_FORMAT_TYPE_INVALID，说明该属性不存在
	 */
	virtual ElementFormatType GetVertexFormat(VertexAttributeType attr, uint8_t index) const = 0;

	size_t GetVertexElementByteSize(VertexAttributeType attr, uint8_t index) const
	{
		return SizeOfElementFormat[GetVertexFormat(attr, index)];
	}

	size_t GetVertexDataByteSize(VertexAttributeType attr, uint8_t index) const
	{
		return GetVertexCount() * GetVertexElementByteSize(attr, index);
	}

	/** 获取索引数据的格式 */
	virtual ElementFormatType GetIndexFormat() const = 0;

	size_t GetIndexElementByteSize() const { return SizeOfElementFormat[GetIndexFormat()]; }

	size_t GetIndexDataByteSize() const
	{
		return GetIndexCount() * GetIndexElementByteSize();
	}

	/** 获取网格名称 */
	virtual const char* GetMeshName() { return "Unknown Mesh"; }
	
	/** 获取网格的加载器(来源) 
	 * @return 返回nullptr，说明来源不明*/
	virtual const IMeshLoader* GetMeshLoader() const { return nullptr; }
};


END_NAME_SPACE
