#pragma once

#include "../UVType.h"
#include "../UVConfig.h"
#include <vector>
#include <map>
#include <string>
#include <array>

BEG_NAME_SPACE

struct BufferDescriptor {
	BufferDescriptor(BufferHandle handle, ResourceStatus status, size_t size)
		: handle(handle), size(size), status(status) {}
	static BufferDescriptor CreateInvalidDescriptor() {
		return BufferDescriptor(BufferHandle::InvalidIndex(), {}, 0);
	}
	const BufferHandle handle;
	const size_t size;
	const ResourceStatus status; /**< 描述用途 */
};

struct TextureDescriptor {
	TextureDescriptor(TextureHandle handle, ResourceStatus status, uint32_t width, uint32_t height, ElementFormatType type)
		: handle(handle), status(status), width(width), height(height), elementFormat(type) {}
	static TextureDescriptor CreateInvalidDescriptor() {
		return TextureDescriptor(TextureHandle::InvalidIndex(), {}, 0, 0, ElementFormatType::ELEMENT_FORMAT_TYPE_INVALID);
	}
	const TextureHandle handle;
	const uint32_t width; /**< 纹理的宽度，单位像素 */
	const uint32_t height; /**< 纹理的高度，单位像素 */
	const ElementFormatType elementFormat; /**< 每个像素的格式 */
	const ResourceStatus status; /**< 描述用途 */
};

///** 通用的采样器描述信息集合 */
//struct SamplerDescriptor {
//	float borderColor[4];
//	FilterType filter;
//	SamplerAddressMode uAddrMode, vAddrMode, wAddrMode;
//	/** 尽量不要使用无参初始化! */
//	SamplerDescriptor() = default;
//	SamplerDescriptor(FilterType filter, SamplerAddressMode u,
//		SamplerAddressMode v, SamplerAddressMode w,
//		const float (&bc)[4])
//		: filter(filter), 
//		uAddrMode(u), vAddrMode(v), wAddrMode(w),
//		borderColor{bc[0], bc[1], bc[2], bc[3]} {}
//};

END_NAME_SPACE
