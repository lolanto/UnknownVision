#pragma once
#include "Shader.h"

#include <functional>
#include <assert.h>

BEG_NAME_SPACE

class CommandUnit;

/** 管线中只能进行固定设置的描述信息集合 */
struct RasterizeOptions {
	CullMode cullMode = CULL_MODE_BACK;
	FillMode fillMode = FILL_MODE_SOLID;
	bool counterClockWiseIsFront = false;
	PrimitiveType primitive = PRIMITIVE_TYPE_TRIANGLE_LIST;
	RasterizeOptions() : cullMode(CULL_MODE_BACK), fillMode(FILL_MODE_SOLID),
		counterClockWiseIsFront(false), primitive(PRIMITIVE_TYPE_TRIANGLE_LIST) {}
};

/** 模板测试，深度测试，混合等输出过程的操作设置 */
struct OutputStageOptions {
	bool enableDepthTest = true;
	BlendOption blending = BLEND_OPTION_NO_BLEND;
	ElementFormatType dsvFormat = ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT; /**< 深度模板缓冲的格式 */
	ElementFormatType rtvFormats[MAX_RENDER_TARGET]; /**< 各个RenderTarget的顶点格式 */
	OutputStageOptions() : enableDepthTest(true), blending(BLEND_OPTION_NO_BLEND),
		dsvFormat(ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT) {
		for (auto& format : rtvFormats) {
			format = ELEMENT_FORMAT_TYPE_INVALID;
		}
	}
};



using RasterizeOptionsFunc = std::function < RasterizeOptions()>;
using OutputStageOptionsFunc = std::function<OutputStageOptions()>;

inline RasterizeOptions GDefaultRasterizeOptions() {
	return RasterizeOptions();
}

inline OutputStageOptions GDefaultOutputStageOptions() {
	OutputStageOptions res;
	res.enableDepthTest = false;
	res.rtvFormats[0] = ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM;
	res.dsvFormat = ELEMENT_FORMAT_TYPE_UNKNOWN;
	return res;
}

/** 描述顶点缓冲中一个属性的子结构 */
struct VertexAttribute {
	enum { APPEND_FROM_PREVIOUS = UINT8_MAX };
	VertexAttributeType type; /**< 描述的顶点属性类型 */
	uint8_t index = 0; /**< 同一个属性中的第几个，如uv0, uv1 */
	ElementFormatType format; /**< 该属性的数据类型 */
	uint8_t location = 0; /**< 该属性被绑定到哪个接口上，管线的"顶点缓冲接口"是有数量上限的 */
	uint8_t byteOffset = 0; /**< 属性的每一个值在缓冲中间隔的距离, 假如为APPEND_FROM_PREVIOUS则表示直接接着上一个属性 */
	VertexAttribute() = default;
	/** 构造一个子顶点属性描述对象
	 * @param type 属性类型
	 * @param format 顶点属性的格式
	 * @param index 当前设置的是该属性数组的第几个元素，比如position0, position1或者normal2之类
	 * @param location 存储该属性的顶点缓冲区会绑定到哪个接口(slot)上
	 * @param byteOffset 该属性的值距离第一个属性值之间间隔的距离，单位是字节。设置为APPEND_FROM_PREVIOUS表示紧接着上一个属性，此时属性的声明顺序十分重要 */
	VertexAttribute(VertexAttributeType type, ElementFormatType format, uint8_t index,
		uint8_t location = 0, uint8_t byteOffset = 0)
		: type(type), index(index), format(format), location(location), byteOffset(byteOffset) {}
};
using VertexAttributesFunc = std::function<std::vector<VertexAttribute>()>;
/** 默认的输入结构包括:
 * position0: float3 存放于slot0
 * texcoord0: float2 存放于slot1 */
inline std::vector<VertexAttribute> GDefaultVertexAttributeList() {
	VertexAttribute position = VertexAttribute(VERTEX_ATTRIBUTE_TYPE_POSITION,
		ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
		0, 0, VertexAttribute::APPEND_FROM_PREVIOUS);
	VertexAttribute texcoord0 = VertexAttribute(VERTEX_ATTRIBUTE_TYPE_TEXTURE,
		ELEMENT_FORMAT_TYPE_R32G32_FLOAT,
		0, 1, VertexAttribute::APPEND_FROM_PREVIOUS);
	return { position, texcoord0 };
}

struct ViewPort {
	float topLeftX, topLeftY;
	float width, height;
	float minDepth, maxDepth;
};

struct ScissorRect {
	size_t left, top, right, bottom;
};

class GraphicsPipelineObject {
public:
	GraphicsPipelineObject(VertexShader* vs, PixelShader* ps,
		RasterizeOptionsFunc rastOpt = GDefaultRasterizeOptions,
		OutputStageOptionsFunc outputOpt = GDefaultOutputStageOptions,
		VertexAttributesFunc vtxAttribList = GDefaultVertexAttributeList)
		: rastOpt(rastOpt), outputOpt(outputOpt), vtxAttribList(vtxAttribList),
		vs(vs), ps(ps) {}
	GraphicsPipelineObject(GraphicsPipelineObject&& rhs) 
		: rastOpt(rhs.rastOpt), outputOpt(rhs.outputOpt), vtxAttribList(rhs.vtxAttribList),
		vs(rhs.vs), ps(rhs.ps) {}
	virtual ~GraphicsPipelineObject() = default;
public:
	/** 由于以下三个结构体并不需要长期存在，所以考虑使用函数生成的方式在使用时直接生成 */
	const RasterizeOptionsFunc rastOpt;
	const OutputStageOptionsFunc outputOpt;
	const VertexAttributesFunc vtxAttribList;

	VertexShader* const vs;
	PixelShader* const ps;
};

END_NAME_SPACE
