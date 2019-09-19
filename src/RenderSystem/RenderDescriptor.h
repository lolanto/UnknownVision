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

/** 通用的采样器描述信息集合 */
struct SamplerDescriptor {
	float borderColor[4];
	FilterType filter;
	SamplerAddressMode uAddrMode, vAddrMode, wAddrMode;
	/** 尽量不要使用无参初始化! */
	SamplerDescriptor() = default;
	SamplerDescriptor(FilterType filter, SamplerAddressMode u,
		SamplerAddressMode v, SamplerAddressMode w,
		const float (&bc)[4])
		: filter(filter), 
		uAddrMode(u), vAddrMode(v), wAddrMode(w),
		borderColor{bc[0], bc[1], bc[2], bc[3]} {}
};

/** 传入command或者program中用到的参数类型
 * 是对资源描述器的统一包装
 * TODO: 是否有更高效的方法将不同类型的参数统一 */
struct Parameter {
	enum Type : uint8_t {
		PARAMETER_TYPE_INVALID = 0u,
		PARAMETER_TYPE_BUFFER,
		PARAMETER_TYPE_TEXTURE,
		PARAMETER_TYPE_SAMPLER
	};
	union
	{
		TextureHandle tex;
		BufferHandle buf;
		SamplerHandle sampler;
	};
	Type type;
	Parameter& operator=(const Parameter& rhs) { 
		type = rhs.type;
		switch (rhs.type) {
			case PARAMETER_TYPE_BUFFER: buf = rhs.buf; break;
			case PARAMETER_TYPE_TEXTURE: tex = rhs.tex; break;
			case PARAMETER_TYPE_SAMPLER: sampler = rhs.sampler; break;
		}
		return *this;
	}
	Parameter() : buf(BufferHandle::InvalidIndex()), type(PARAMETER_TYPE_INVALID) {}
	Parameter(const BufferHandle& buf) : buf(buf), type(PARAMETER_TYPE_BUFFER) {}
	Parameter(const TextureHandle& tex) : tex(tex), type(PARAMETER_TYPE_TEXTURE) {}
	Parameter(const SamplerHandle& sampler) : sampler(sampler), type(PARAMETER_TYPE_SAMPLER) {}
};

using VertexAttributeDescs = std::vector<SubVertexAttributeDesc>;

/** 专门用于存储shader名称
 * @remark 名称为Shader的源码路径，不带后缀! */
struct ShaderNames {
	std::array<std::string, SHADER_TYPE_NUMBER_OF_TYPE> names;
	ShaderNames() {}
	ShaderNames(const char* vs, const char* ps) {
		names[SHADER_TYPE_VERTEX_SHADER] = vs;
		names[SHADER_TYPE_PIXEL_SHADER] = ps;
	}
	ShaderNames(const char* cs) {
		names[SHADER_TYPE_COMPUTE_SHADER] = cs;
	}
	ShaderNames(ShaderNames&& rhs) {
		names.swap(rhs.names);
	}
	ShaderNames(const ShaderNames& rhs) {
		names = rhs.names;
	}
	void swap(ShaderNames&& rhs) noexcept {
		names.swap(rhs.names);
	}
	void swap(ShaderNames& rhs) noexcept {
		names.swap(rhs.names);
	}
	decltype(names)::reference operator[](uint8_t index) { return names[index]; }
	decltype(names)::const_reference operator[](uint8_t index) const { return names[index]; }
};


/** 管线/compute程序，记录了该程序的输入输出等信息 */
struct ProgramDescriptor {
	mutable std::map<std::string, Parameter::Type> signature; /**< 程序的签名，记录了参数和参数在shader中的名称 */
	mutable ShaderNames shaders; /**< shader的名称和shader类型 */
	const ProgramHandle handle;
	const ProgramType type;
	const bool usedIndex; /**< 程序是否需要使用索引 */
	const RasterizeOptions rastOpt; /**< 光栅过程的设置 */
	const OutputStageOptions osOpt; /**< 模板，深度以及混合过程的设置 */
	/** 默认构造函数用于容器 */
	ProgramDescriptor()
		: handle(ProgramHandle::InvalidIndex()), type(PROGRAM_TYPE_GRAPHICS), usedIndex(false) {}
	ProgramDescriptor(std::map<std::string, Parameter::Type>&& signature,
		ShaderNames shaders, ProgramHandle handle,
		ProgramType type, bool usedIndex, RasterizeOptions rastOpt, OutputStageOptions osOpt)
		: signature(std::move(signature)), shaders(shaders), handle(handle), type(type), usedIndex(usedIndex),
		rastOpt(rastOpt), osOpt(osOpt) {}
	ProgramDescriptor(ProgramDescriptor&& rhs)
		: handle(rhs.handle), type(rhs.type), usedIndex(rhs.usedIndex),
		rastOpt(rhs.rastOpt), osOpt(rhs.osOpt) {
		signature.swap(rhs.signature);
		shaders.swap(rhs.shaders);
	}
	ProgramDescriptor(const ProgramDescriptor& rhs)
		: handle(rhs.handle), type(rhs.type), usedIndex(rhs.usedIndex), rastOpt(rhs.rastOpt),
		osOpt(rhs.osOpt), signature(rhs.signature), shaders(rhs.shaders) { }
	static ProgramDescriptor CreateInvalidDescirptor() thread_safe { return ProgramDescriptor(); }
};

END_NAME_SPACE
