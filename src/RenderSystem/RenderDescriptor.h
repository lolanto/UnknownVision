#pragma once

#include "../UVType.h"
#include "../UVConfig.h"

BEG_NAME_SPACE

struct BufferDescriptor {
	BufferDescriptor(BufferHandle handle, ResourceStatus status, size_t size)
		: handle(handle), size(size), status(status) {}
	static BufferDescriptor CreateInvalidDescriptor() {
		return BufferDescriptor(BufferHandle::InvalidIndex(), {}, 0);
	}
	const BufferHandle handle;
	const size_t size;
	const ResourceStatus status;
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
	const ResourceStatus status;
};

END_NAME_SPACE
