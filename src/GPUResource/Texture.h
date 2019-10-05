#ifndef UV_TEXTURE_H
#define UV_TEXTURE_H

#include "Resource.h"

namespace UnknownVision {
	class Texture : public Resource {
	public:
		Texture(uint32_t flag, DataFormatType type, uint32_t RID) : Resource(RID),
			m_flag(flag), m_eleType(type) {}
		~Texture() = default;
		DataFormatType ElementType() const { return m_eleType; }
		TextureFlagCombination Flag() const { return m_flag; }
	protected:
		DataFormatType m_eleType = DFT_INVALID;
		TextureFlagCombination m_flag = DFT_INVALID;
	};

	class Texture2D : public Texture {
	public:
		Texture2D(float width, float height,
			uint32_t flag, DataFormatType type, uint32_t RID)
			: Texture(flag, type, RID) {}
		~Texture2D() = default;
		float Width() const { return m_width; }
		float Height() const { return m_height; }
	private:
		float m_width = 0;
		float m_height = 0;
	};
}

#endif // UV_TEXTURE_H
