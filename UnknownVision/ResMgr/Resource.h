#ifndef RESOURCE_H
#define RESOURCE_H
#include "ResMgr_UVConfig.h"

// 针对资源的抽象基类
/* 无论何种Resource对象
均不包含资源对应的图形库的相关内容
与图形库相关的内容存储在管理器中
资源对象仅包含资源的属性以供查询
*/
namespace UnknownVision {
	class Resource {
	public:
		Resource(uint32_t id) : RID(id) {}
		const uint32_t RID;
	};

	class Texture : public Resource {
	public:
		Texture(uint32_t flag, TextureElementType type, uint32_t RID) : Resource(RID),
			m_flag(flag), m_eleType(type) {}
		TextureElementType ElementType() const { return m_eleType; }
		TextureFlagCombination Flag() const { return m_flag; }
	protected:
		TextureElementType m_eleType = TET_INVALID;
		TextureFlagCombination m_flag = TF_INVALID;
	};

	class Texture2D : public Texture {
	public:
		Texture2D(float width, float height,
			uint32_t flag, TextureElementType type, uint32_t RID)
			: Texture(flag, type, RID) {}
		float Width() const { return m_width; }
		float Height() const { return m_height; }
	private:
		float m_width = 0;
		float m_height = 0;
	};

	class Buffer : public Resource {
	public:
		Buffer(size_t byteSize, size_t numEle, BufferType type, BufferFlagCombination flag, uint32_t RID)
			: m_byteSize(byteSize), m_numEle(numEle), m_type(type), m_flag(flag), Resource(RID) {}

		size_t ByteSize() const { return m_byteSize; }
		size_t NumberOfElements() const { return m_numEle; }
		size_t ElementSize() const { return m_byteSize / m_numEle; }
		BufferType Type() const { return m_type; }
		BufferFlagCombination Flag() const { return m_flag; }
	protected:
		const size_t m_byteSize; // 整个缓冲的大小
		const size_t m_numEle; // 缓冲中的元素数量
		const BufferFlagCombination m_flag = BufferFlag::BF_INVALID;
		const BufferType m_type;
	};

	class Shader : public Resource {
	public:
		Shader(ShaderType type, uint32_t RID)
			: Type(type), Resource(RID) {}

		const ShaderType Type;
	};

	class VertexDeclaration : public Resource {
	public:
		VertexDeclaration(uint32_t RID)
			: Resource(RID) {}
	};
}

#endif // RESOURCE_H
