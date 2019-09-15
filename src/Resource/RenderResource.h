#pragma once

#include "../UVType.h"
#include "ShaderResource.h"

BEG_NAME_SPACE

class RenderResource {
	virtual ConstantBufferView* GetCBVPtr() { return nullptr };
	virtual ShaderResourceView* GetSRVPtr() { return nullptr };
};

class Texture {

};

class Texture2D : public Texture {
protected:
	size_t m_width;
	size_t m_height;
};

class Buffer : public RenderResource {
public:
	virtual bool Initialize(ResourceStatus status) = 0;
	virtual void Reset() = 0;
	size_t& Size() { return m_size; }
	size_t Size() const { return m_size; }
protected:
	size_t m_size;
	ResourceStatus m_status;
};

END_NAME_SPACE
