#ifndef RESOURCE_H
#define RESOURCE_H
#include "RenderSys_UVConfig.h"
#include <string>
#include <vector>
#include <memory>

// 针对资源的抽象基类
/* 无论何种Resource对象
均不包含资源对应的图形库的相关内容
与图形库相关的内容存储在管理器中
资源对象仅包含资源的属性以供查询
*/
namespace UnknownVision {
	class Resource {
	public:
		Resource(UINT id) : RID(id) {}
		const UINT RID;
	};

	class Texture : public Resource {
	public:
		Texture(TextureFlag flag, TextureElementType type, UINT RID) : Resource(RID),
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
			TextureFlag flag, TextureElementType type, UINT RID)
			: Texture(flag, type, RID) {}
		float Width() const { return m_width; }
		float Height() const { return m_height; }
	private:
		float m_width = 0;
		float m_height = 0;
	};
}

#endif // RESOURCE_H
