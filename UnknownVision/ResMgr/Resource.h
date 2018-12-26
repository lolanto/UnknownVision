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
		Texture(TextureFlag flag, TextureElementType type, uint32_t RID) : Resource(RID),
			m_flag(flag), m_eleType(type) {}
		TextureElementType ElementType() const { return m_eleType; }
		TextureFlag Flag() const { return m_flag; }
	protected:
		TextureElementType m_eleType = TET_INVALID;
		TextureFlag m_flag = TF_INVALID;
	};

	class Texture2D : public Texture {
	public:
		Texture2D(float width, float height,
			TextureFlag flag, TextureElementType type, uint32_t RID)
			: Texture(flag, type, RID) {}
		float Width() const { return m_width; }
		float Height() const { return m_height; }
	private:
		float m_width = 0;
		float m_height = 0;
	};

	class Buffer : public Resource {
	public:
		Buffer(size_t size, BufferFlag flag, uint32_t RID)
			: Resource(RID) {}
		size_t ByteSize() const { return m_byteSize; }
		BufferFlag Flag() const { return m_flag; }
	protected:
		size_t m_byteSize;
		BufferFlag m_flag;
	};

	class VertexBuffer : public Buffer {
	public:
		VertexBuffer(size_t numVtxs, size_t sizeVtx,
			BufferFlag flag, uint32_t RID)
			: m_numVtxs(numVtxs), m_sizeVtx(sizeVtx), Buffer(numVtxs * sizeVtx, flag, RID) {}
		size_t NumOfVertex() const { return m_numVtxs; }
		size_t SizeOfVertex() const { return m_sizeVtx; }
	private:
		size_t m_numVtxs; // number of vertex
		size_t m_sizeVtx; // size of vertex in byte
	};

	class Shader : public Resource {
	public:
		Shader(ShaderType type, uint32_t RID)
			: m_type(type), Resource(RID) {}
	protected:
		ShaderType m_type;
	};
}

#endif // RESOURCE_H
