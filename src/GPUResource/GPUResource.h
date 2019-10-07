#pragma once

#include "../UVType.h"

BEG_NAME_SPACE

class CommandUnit;
class RenderDevice;
/** 创建资源时填写的资源申请表 */
struct RenderResourceCreationDescription {
	/** 资源中长宽高各个轴向上包含的元素数量 */
	size_t width;
	size_t height;
	size_t slice;
	ResourceUsages usages; /**< 创建的资源的用途，可以使用逻辑or选择多个用途 */
	ResourceFlags flags; /**< 目前是cpu对资源的操作权限描述 */
	union {
		ElementFormatType format; /**< 资源的元素格式 */
		uint8_t elementStride; /**< 对于任意格式资源，需要给定一个元素的大小，针对缓冲区 */
	};
	
	/** 构造函数 */
	RenderResourceCreationDescription(size_t width = 0, size_t height = 0, size_t slice = 0,
		ResourceUsages usages = RESOURCE_USAGE_INVALID,
		ResourceFlags flags = RESOURCE_FLAG_INVALID,
		ElementFormatType format = ELEMENT_FORMAT_TYPE_INVALID)
		: width(width), height(height), slice(slice), usages(usages), flags(flags), format(format) {}

	/** 创建任意元素大小的缓冲区，比如顶点缓冲，结构化缓冲，常量缓冲 */
	static RenderResourceCreationDescription Buffer(size_t numElements, ResourceUsages usages,
		ResourceFlags flags, uint8_t elementStride) {
		auto res = RenderResourceCreationDescription(numElements, 0, 0, usages, flags);
		res.elementStride = elementStride;
		return res;
	}
	static RenderResourceCreationDescription Texture2D(size_t width, size_t height, ResourceUsages usages,
		ResourceFlags flags, ElementFormatType format) {
		return RenderResourceCreationDescription(width, height, 0, usages, flags, format);
	}
};

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
public:
	virtual void SetName(const wchar_t* name) {}
	virtual void* GetResource() { return nullptr; }
	virtual ConstantBufferView* GetCBVPtr() { return nullptr; }
	virtual ShaderResourceView* GetSRVPtr() { return nullptr; }
	virtual UnorderAccessView* GetUAVPtr() { return nullptr; }
	virtual RenderTargetView* GetRTVPtr() { return nullptr; }
	virtual DepthStencilView* GetDSVPtr() { return nullptr; }
	virtual VertexBufferView* GetVBVPtr() { return nullptr; }
	virtual IndexBufferView* GetIBVPtr() { return nullptr; }
	virtual bool Avaliable() const { return false; }
	/** 请求临时资源，该资源会在对应的CommandUnit执行完指令后被释放 */
	virtual bool RequestTransient(CommandUnit* cmdUnit) { return false; };
	/** 请求固定的资源，资源的释放需要手动控制，或者等到程序结束后释放 */
	virtual bool RequestPermenent(CommandUnit* cmdUnit) { return false; };
	/** 用来手动释放资源，临时资源也可以提前进行手动释放，保证释放空资源不会有影响 */
	virtual void Release() = 0;
	/** 确认该资源是否是长期存储的 */
	bool IsPermenent() { return m_bPermenent; }
protected:
	ResourceStatus m_status;
	ResourceStates m_state;
	bool m_bPermenent;
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
		const float(&bc)[4])
		: filter(filter),
		uAddrMode(u), vAddrMode(v), wAddrMode(w),
		borderColor{ bc[0], bc[1], bc[2], bc[3] } {}
};

END_NAME_SPACE
