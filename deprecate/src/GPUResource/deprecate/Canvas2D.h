#pragma once
#include "GPUResource.h"

BEG_NAME_SPACE

/** 包含了可以被GPU读写的二维纹理 */

/** CPU不可读写的二维Canvas */
class DiscreteCanvas2D : public GPUResource {
public:
	DiscreteCanvas2D() :
		m_width(0), m_height(0), m_format(ELEMENT_FORMAT_TYPE_INVALID) {
		m_status.usage = RESOURCE_USAGE_RENDER_TARGET | RESOURCE_USAGE_SHADER_RESOURCE;
		m_status.flag = RESOURCE_FLAG_STABLY;
	}
	virtual ~DiscreteCanvas2D() = default;
public:
	size_t Width() const { return m_width; }
	size_t Height() const { return m_height; }
	ElementFormatType Format() const { return m_format; }
	void SetInitialData(size_t width, size_t height, ElementFormatType format) {
		m_width = width;
		m_height = height;
		m_format = format;
	}
	/** 用来手动释放资源，临时资源也可以提前进行手动释放，保证释放空资源不会有影响 */
	virtual void Release() {
		m_width = 0;
		m_height = 0;
		m_format = ELEMENT_FORMAT_TYPE_INVALID;
	}
protected:
	size_t m_width;
	size_t m_height;
	ElementFormatType m_format;
};


END_NAME_SPACE
