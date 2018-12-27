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
		uint32_t Flag() const { return m_flag; }
	protected:
		TextureElementType m_eleType = TET_INVALID;
		uint32_t m_flag = TF_INVALID;
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
		Buffer(size_t size, size_t numEle, uint32_t flag, uint32_t RID)
			: m_byteSize(size), m_numEle(numEle), m_flag(flag), Resource(RID) {}

		size_t ByteSize() const { return m_byteSize; }
		size_t NumEle() const { return m_numEle; }
		uint32_t Flag() const { return m_flag; }
	protected:
		size_t m_byteSize;
		size_t m_numEle;
		uint32_t m_flag;
	};

	class Shader : public Resource {
	public:
		Shader(ShaderType type, uint32_t RID)
			: Type(type), Resource(RID) {}

		const ShaderType Type;
	};
}

#endif // RESOURCE_H
