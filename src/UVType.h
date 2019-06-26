#pragma once
#include "UVConfig.h"

BEG_NAME_SPACE

/** 所有特殊资源与设备相关，必须在设备初始化中完成 */

enum SpecialTextureResource : uint8_t {
	DEFAULT_BACK_BUFFER = 0,
	NUMBER_OF_SPECIAL_TEXTURE_RESOURCE
};

enum SpecialBufferResource : uint8_t {
	NUMBER_OF_SPECIAL_BUFFER_RESOURCE = 0
};

/** 资源的使用方式，通常用于显式规定资源的用途 */
allow_logical_operation enum ResourceUsage : uint8_t {
	RESOURCE_USAGE_INVALID = 0X00U,
	RESOURCE_USAGE_VERTEX_BUFFER = 0x01U,
	RESOURCE_USAGE_INDEX_BUFFER = 0x02U,
	RESOURCE_USAGE_DEPTH = 0x04U,
	RESOURCE_USAGE_RENDER_TARGET = 0x08U,
	RESOURCE_USAGE_SHADER_RESOURCE = 0x10U,
	RESOURCE_USAGE_CONSTANT_BUFFER = 0x20U,
	RESOURCE_USAGE_UNORDER_ACCESS = 0x40U
};

allow_logical_operation enum ResourceFlag : uint8_t {
	RESOURCE_FLAG_INVALID = 0x00U,
	RESOURCE_FLAG_STABLY = 0x01U, /**< CPU will never read / write */
	RESOURCE_FLAG_ONCE = 0x02U, /**< CPU will write per frame */
	RESOURCE_FLAG_FREQUENTLY = 0x04U, /**< CPU will write multiple time per frame */
	RESOURCE_FLAG_READ_BACK = 0x08U, /**< CPU will read it */
};


#define CanBe(x, X) inline bool canBe##x() const { return usage & RESOURCE_USAGE_##X; }
#define IsFlag(x, X) inline bool is##x() const { return flag & RESOURCE_FLAG_##X; }
#define IsInState(x, X) inline bool isInStateOf##x() const { return state == RESOURCE_STATE_##X; }
struct ResourceStatus {
	ResourceUsage usage = RESOURCE_USAGE_INVALID;
	ResourceFlag flag = RESOURCE_FLAG_INVALID;

	ResourceStatus() = default;
	ResourceStatus(ResourceUsage usage, ResourceFlag flag)
		:usage(usage), flag(flag) {}
	/** helper functions */
	inline bool isInvalid() const {
		return usage == RESOURCE_USAGE_INVALID ||
			flag == RESOURCE_FLAG_INVALID;
	}
	IsFlag(Stably, STABLY);
	IsFlag(Once, ONCE);
	IsFlag(Frequently, FREQUENTLY);
	IsFlag(ReadBack, READ_BACK);
	CanBe(VertexBuffer, VERTEX_BUFFER);
	CanBe(IndexBuffer, INDEX_BUFFER);
	CanBe(ConstantBuffer, CONSTANT_BUFFER);
};
#undef IsInState
#undef IsFlag
#undef CanBe

enum ElementFormatType : uint8_t {
	ELEMENT_FORMAT_TYPE_INVALID = 0,/**< 无效的默认值 */
	/** 以下格式可以等价为float1, float2, float3以及float4 */
	ELEMENT_FORMAT_TYPE_R32_FLOAT,
	ELEMENT_FORMAT_TYPE_R32G32_FLOAT,
	ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
	ELEMENT_FORMAT_TYPE_R32G32B32A32_FLOAT,
	ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM, /**< 常用于描述渲染对象元素的格式 */
	/**/
	ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT, /**< 常用于描述深度模板缓存元素的格式 */
};

struct SubVertexAttributeDesc {
	const char* semantic = nullptr;
	uint8_t index = 0;
	ElementFormatType dataType;
	uint8_t bufIdx = 0;
	uint8_t byteOffset = 0;
};


/** 图元类型的枚举值，与光栅化相关
*/
enum Primitive {
	PRI_INVALID, /**< 无效图元类型 */
	PRI_Point, /**< 点图元 */
	PRI_TriangleList /**< 三角形列表图元 */
};

enum API_TYPE {
	DirectX11_0 = 0,
	DirectX12_0 = 1
};

/** 视口设置描述对象 */
struct ViewPortDesc {
	float topLeftX = 0.0f; /**< 视口的左上角横坐标，单位为像素 */
	float topLeftY = 0.0f; /**< 视口的左上角纵坐标，单位为像素 */
	float width = 0.0f; /**< 视口的宽度，单位为像素 */
	float height = 0.0f; /**< 视口的高度，单位为像素 */
	float minDepth = 0.0f; /**< 深度值最小值，范围0~1 */
	float maxDepth = 1.0f; /**< 深度值最大值，范围0~1*/
};

enum ShaderType {
	SHADER_TYPE_VERTEX_SHADER,
	SHADER_TYPE_PIXEL_SHADER,
	SHADER_TYPE_GEOMETRY_SHADER,
	SHADER_TYPE_COMPUTE_SHADER
};

enum BufferFlag : uint32_t {
	BF_INVALID = 0,
	BF_WRITE_BY_CPU = 0x00000001U, // CPU能够写
	BF_READ_BY_CPU = 0x00000002U, // CPU能够读
	BF_WRITE_BY_GPU = 0x00000004U,
	BF_READ_BY_GPU = 0x00000008U
};

enum BufferType : uint32_t {
	BT_VERTEX_BUFFER,
	BT_INDEX_BUFFER,
	BT_CONSTANT_BUFFER
};

enum TextureFlag : uint32_t {
	TF_INVALID = 0x00000000U,
	TF_READ_BY_GPU = 0x00000001U,
	TF_WRITE_BY_GPU = 0x00000002U,
	TF_READ_BY_CPU = 0x00000004U,
	TF_WRITE_BY_CPU = 0x00000008U
};

using TextureFlagCombination = uint32_t;
using BufferFlagCombination = uint32_t;

/** 为索引值设置别名，加强类型检查 */
ALIAS_INDEX(int32_t, Texture2DIdx);
ALIAS_INDEX(int32_t, RenderTargetIdx);
ALIAS_INDEX(int32_t, DepthStencilIdx);
ALIAS_INDEX(int32_t, ShaderIdx);
ALIAS_INDEX(int32_t, BufferIdx);
ALIAS_INDEX(int32_t, VertexDeclarationIdx);
ALIAS_INDEX(int32_t, PipelineStateIdx);
ALIAS_INDEX(int32_t, ResourceIdx);
ALIAS_INDEX(int32_t, DescriptorLayoutIdx);
ALIAS_INDEX(int32_t, GraphPassIdx);
ALIAS_INDEX(uint64_t, BufferHandle);
ALIAS_INDEX(uint64_t, TextureHandle);
ALIAS_INDEX(uint64_t, ProgramHandle);
ALIAS_INDEX(uint64_t, TaskFrame);

END_NAME_SPACE
