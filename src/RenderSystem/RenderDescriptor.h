#pragma once

#include "../UVType.h"
#include "../UVConfig.h"
#include <vector>
#include <map>
#include <string>

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
/** 传入command或者program中用到的参数类型
 * 是对资源描述器的统一包装
 * TODO: 是否有更高效的方法将不同类型的参数统一 */
struct Parameter {
	enum Type : uint8_t {
		PARAMETER_TYPE_INVALID = 0u,
		PARAMETER_TYPE_BUFFER
	};
	union
	{
		BufferDescriptor buf;
	};
	const Type type;
	Parameter(const BufferDescriptor& buf) : buf(buf), type(PARAMETER_TYPE_BUFFER) {}
};

typedef std::vector<SubVertexAttributeDesc> VertexAttributeDescs;

/** 一个program执行时候可选的执行方式 */
struct ProgramOptions {
	DepthStencilOptions depthStencilOpts;
	RasterizeOptions rasterOpts;
	BlendOption blendOpt;
};

struct ShaderNames {
	std::string vs;
	std::string ps;
	std::string gs;
	std::string ts;
	std::string cs;
	ShaderNames()
		: vs(""), ps(""), gs(""), ts(""), cs("") {}
	ShaderNames(const char* vs, const char* ps)
		: vs(vs), ps(ps), gs(""), ts(""), cs("") {}
	ShaderNames(const char* cs)
		: vs(""), ps(""), gs(""), ts(""), cs(cs) {}
};

/** 管线/compute程序，记录了该程序的输入输出等信息 */
struct ProgramDescriptor {
	const std::map<std::string, Parameter> signature; /**< 程序的签名，记录了参数和参数在shader中的名称 */
	const ShaderNames shaders; /**< shader的名称和shader类型 */
	const ProgramHandle handle;
	const ProgramType type;
	const bool usedIndex; /**< 程序是否需要使用索引 */

	/** 默认构造函数用于容器 */
	ProgramDescriptor()
		: handle(ProgramHandle::InvalidIndex()), type(PROGRAM_TYPE_GRAPHICS), usedIndex(false) {}
	ProgramDescriptor(std::map<std::string, Parameter> signature,
		ShaderNames shaders, ProgramHandle handle,
		ProgramType type, bool usedIndex)
		: signature(signature), shaders(shaders), handle(handle), type(type), usedIndex(usedIndex) {}
};

END_NAME_SPACE
