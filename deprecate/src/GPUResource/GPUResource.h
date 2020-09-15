#pragma once

#include "../UVType.h"
#include <assert.h>

BEG_NAME_SPACE

class CommandUnit;
class RenderDevice;

/** 使用描述的方式创建资源，相当于抑制了之后资源的用途更改可能
 * 虽然这符合目前各个图形API的创建规范，但自由度不高
 * 最显著的好处是方便资源的管理，提前预设不允许改变可以减少应对改变的实现成本*/

class ConstantBufferView;
class ShaderResourceView;
class UnorderAccessView;
class RenderTargetView;
class DepthStencilView;
class VertexBufferView;
class IndexBufferView;

class GPUResource : public Uncopyable {
	friend class DX12CommandUnit;
	friend class DX12RenderDevice;
public:
	virtual ~GPUResource() { if (Avaliable()) Release(); }
	virtual void SetName(const wchar_t* name) {}
	virtual void* GetResource() { return nullptr; }
	virtual bool Avaliable() const { return false; }
	virtual GPUResourceType Type() const { return GPU_RESOURCE_TYPE_INVALID; }
	/** 用来手动释放资源，临时资源也可以提前进行手动释放，保证释放空资源不会有影响
	 * 即便没有手动调用释放，析构函数也保证该函数被调用 */
	virtual void Release() { assert(false); };
protected:
	ResourceStatus m_status;
	ResourceStates m_state;
};

class Buffer : public GPUResource {
	friend class DX12RenderDevice;
public:
	Buffer() = default;
	virtual ~Buffer() = default;
	size_t Capacity() const { return m_capacity; }
	size_t StrideInBytes() const { return m_strideInBytes; }
	size_t MemFootprint() const { return m_capacity * m_strideInBytes; }
	virtual GPUResourceType Type() const override final { return GPU_RESOURCE_TYPE_BUFFER; }
protected:
	size_t m_capacity;
	size_t m_strideInBytes;
};

class Texture : public GPUResource {
	friend class DX12RenderDevice;
public:
	Texture() = default;
	virtual ~Texture() = default;
	ElementFormatType Format() const { return m_format; }
protected:
	ElementFormatType m_format;
};

class Texture2D : public Texture {
	friend class DX12RenderDevice;
public:
	Texture2D() = default;
	virtual ~Texture2D() = default;
	size_t Width() const { return m_width; }
	size_t Height() const { return m_height; }
	size_t MipLevels() const { return m_miplevels; }
	virtual GPUResourceType Type() const override final { return GPU_RESOURCE_TYPE_TEXTURE2D; }
protected:
	size_t m_width;
	size_t m_height;
	size_t m_miplevels;
};

/** 通用的采样器描述信息集合 */
struct SamplerDescriptor {
	float borderColor[4];
	FilterType filter;
	SamplerAddressMode uAddrMode, vAddrMode, wAddrMode;
	/** 尽量不要使用无参初始化! */
	SamplerDescriptor() = default;
	SamplerDescriptor(FilterType filter, SamplerAddressMode u,
		SamplerAddressMode v, SamplerAddressMode w,
		const float(&bc)[4] = {0.0f, 0.0f, 0.0f, 0.0f})
		: filter(filter),
		uAddrMode(u), vAddrMode(v), wAddrMode(w),
		borderColor{ bc[0], bc[1], bc[2], bc[3] } {}
};

END_NAME_SPACE
