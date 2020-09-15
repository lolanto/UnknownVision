#pragma once
#include "UVUtility.h"
#include <tuple>

BEG_NAME_SPACE

using DX12BackendUsedData = std::tuple<size_t, uint32_t, uint32_t>; /**< HWND but cast to void*, windows width, windows height */


/** 资源的使用方式，通常用于显式规定资源的用途 */
allow_logical_operation enum ResourceUsages : uint8_t {
	RESOURCE_USAGE_INVALID = 0X00U,
	RESOURCE_USAGE_VERTEX_BUFFER = 0x01U,
	RESOURCE_USAGE_INDEX_BUFFER = 0x02U,
	RESOURCE_USAGE_DEPTH_STENCIL = 0x04U,
	RESOURCE_USAGE_RENDER_TARGET = 0x08U,
	RESOURCE_USAGE_SHADER_RESOURCE = 0x10U,
	RESOURCE_USAGE_CONSTANT_BUFFER = 0x20U,
	RESOURCE_USAGE_UNORDER_ACCESS = 0x40U,
	RESOURCE_USAGE_STRUCTURED_BUFFER = 0x80U
};

ENUM_LOGICAL_OPERATION(ResourceUsages, uint8_t);

allow_logical_operation enum ResourceFlags : uint8_t {
	RESOURCE_FLAG_INVALID = 0x00U,
	RESOURCE_FLAG_STABLY = 0x01U, /**< CPU will never read / write */
	RESOURCE_FLAG_ONCE = 0x02U, /**< CPU will write per frame */
	RESOURCE_FLAG_FREQUENTLY = 0x04U, /**< CPU will write multiple time per frame */
	RESOURCE_FLAG_READ_BACK = 0x08U, /**< CPU will read it */
};

allow_logical_operation enum ResourceStates : uint32_t {
	RESOURCE_STATE_COMMON = 0,
	RESOURCE_STATE_VERTEX_BUFFER = 0x0001u,
	RESOURCE_STATE_CONSTANT_BUFFER = 0x0002u,
	RESOURCE_STATE_INDEX_BUFFER = 0x0004u,
	RESOURCE_STATE_RENDER_TARGET = 0x0008u,
	RESOURCE_STATE_UNORDER_ACCESS = 0x0010u,
	RESOURCE_STATE_DEPTH_READ = 0x0020u,
	RESOURCE_STATE_DEPTH_WRITE = 0x0040u,
	RESOURCE_STATE_SHADER_RESOURCE = 0x0080u,
	RESOURCE_STATE_COPY_DEST = 0x0100u,
	RESOURCE_STATE_COPY_SRC = 0x0200u,
	RESOURCE_STATE_PRESENT = 0x0400u
};
ENUM_LOGICAL_OPERATION(ResourceStates, uint32_t)

#define CanBe(x, X) inline bool canBe##x() const { return usage & RESOURCE_USAGE_##X; }
#define IsFlag(x, X) inline bool is##x() const { return flag & RESOURCE_FLAG_##X; }
#define IsInState(x, X) inline bool isInStateOf##x() const { return state == RESOURCE_STATE_##X; }
struct ResourceStatus {
	ResourceUsages usage = RESOURCE_USAGE_INVALID;
	ResourceFlags flag = RESOURCE_FLAG_INVALID;

	ResourceStatus() = default;
	ResourceStatus(ResourceUsages usage, ResourceFlags flag)
		:usage(usage), flag(flag) {}
	/** helper functions */
	IsFlag(Stably, STABLY);
	IsFlag(Once, ONCE);
	IsFlag(Frequently, FREQUENTLY);
	IsFlag(ReadBack, READ_BACK);
	CanBe(VertexBuffer, VERTEX_BUFFER);
	CanBe(IndexBuffer, INDEX_BUFFER);
	CanBe(ConstantBuffer, CONSTANT_BUFFER);
	CanBe(RenderTarget, RENDER_TARGET);
	CanBe(DepthStencil, DEPTH_STENCIL);
	CanBe(UnorderAccess, UNORDER_ACCESS);
	CanBe(ShaderResource, SHADER_RESOURCE);
};
#undef IsInState
#undef IsFlag
#undef CanBe

enum GPUResourceType : uint8_t {
	GPU_RESOURCE_TYPE_INVALID,
	GPU_RESOURCE_TYPE_TEXTURE2D,
	GPU_RESOURCE_TYPE_TEXTURE3D,
	GPU_RESOURCE_TYPE_BUFFER,
};

enum ShaderParameterType : uint8_t {
	SHADER_PARAMETER_TYPE_TEXTURE_R,
	SHADER_PARAMETER_TYPE_TEXTURE_RW,
	SHADER_PARAMETER_TYPE_BUFFER_R,
	SHADER_PARAMETER_TYPE_BUFFER_RW,
	SHADER_PARAMETER_TYPE_SAMPLER
};

enum ShaderParameterFlag : uint8_t {
	SHADER_PARAMETER_FLAG_NONE,
	SHADER_PARAMETER_FLAG_CUBE,
	SHADER_PARAMETER_FLAG_ARRAY
};

enum RenderResourceViewType : uint8_t {
	RENDER_RESOURCE_VIEW_CBV = 0,
	RENDER_RESOURCE_VIEW_SRV,
	RENDER_RESOURCE_VIEW_UAV,
	RENDER_RESOURCE_VIEW_RTV,
	RENDER_RESOURCE_VIEW_DSV,


	RENDER_RESOURCE_VIEW_NUM
};

enum ElementFormatType : uint8_t {
	ELEMENT_FORMAT_TYPE_INVALID = 0,/**< 无效的默认值 */
	ELEMENT_FORMAT_TYPE_UNKNOWN,
	/** 以下格式可以等价为float1, float2, float3以及float4 */
	ELEMENT_FORMAT_TYPE_R16_FLOAT,
	ELEMENT_FORMAT_TYPE_R32_FLOAT,
	ELEMENT_FORMAT_TYPE_R32G32_FLOAT,
	ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT,
	ELEMENT_FORMAT_TYPE_R32G32B32A32_FLOAT,
	ELEMENT_FORMAT_TYPE_R16G16B16A16_FLOAT,
	ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM, /**< 常用于描述渲染对象元素的格式 */
	ELEMENT_FORMAT_TYPE_R8_UNORM,
	/**/
	ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT, /**< 常用于描述深度模板缓存元素的格式 */
};

enum FilterType : uint8_t {
	FILTER_TYPE_MIN_MAG_MIP_POINT = 0,
	FILTER_TYPE_MIN_MAG_MIP_LINEAR,
	FILTER_TYPE_ANISOTROPIC
};

enum SamplerAddressMode : uint8_t {
	SAMPLER_ADDRESS_MODE_WRAP = 0,
	SAMPLER_ADDRESS_MODE_CLAMP,
	SAMPLER_ADDRESS_MODE_BORDER
};

/** 顶点的属性类型 */
enum VertexAttributeType : uint8_t {
	VERTEX_ATTRIBUTE_TYPE_POSITION = 0,
	VERTEX_ATTRIBUTE_TYPE_NORMAL,
	VERTEX_ATTRIBUTE_TYPE_TANGENT,
	VERTEX_ATTRIBUTE_TYPE_TEXTURE,
	VERTEX_ATTRIBUTE_TYPE_COLOR
};

/** 混合等式的具体操作 */
enum BlendOperation : uint8_t {
	BLEND_OPERATION_ADD,
	BLEND_OPERATION_SUBTRACT
};

/** 固定的混合操作类型 */
enum BlendOption : uint8_t {
	BLEND_OPTION_ONE,
	BLEND_OPTION_ZERO,
	BLEND_OPTION_SRC_ALPHA,
	BLEND_OPTION_INV_SRC_ALPHA,
	/** TODO: 还需要支持更多 */
};
/** 面的裁剪方式 */
enum CullMode : uint8_t {
	CULL_MODE_BACK = 0,
	CULL_MODE_FRONT,
	CULL_MODE_NONE
};
/** 面元的填充方式 */
enum FillMode : uint8_t {
	FILL_MODE_SOLID = 0,
	FILL_MODE_WIREFRAME
};

/** 图元类型的枚举值，与光栅化相关 */
enum PrimitiveType {
	PRIMITIVE_TYPE_INVALID = 0, /**< 无效图元类型 */
	PRIMITIVE_TYPE_POINT, /**< 点图元 */
	PRIMITIVE_TYPE_TRIANGLE_LIST /**< 三角形列表图元 */
};

//enum API_TYPE {
//	DirectX11_0 = 0,
//	DirectX12_0 = 1
//};

/** 视口设置描述对象 */
struct ViewPortDesc {
	float topLeftX = 0.0f; /**< 视口的左上角横坐标，单位为像素 */
	float topLeftY = 0.0f; /**< 视口的左上角纵坐标，单位为像素 */
	float width = 0.0f; /**< 视口的宽度，单位为像素 */
	float height = 0.0f; /**< 视口的高度，单位为像素 */
	float minDepth = 0.0f; /**< 深度值最小值，范围0~1 */
	float maxDepth = 1.0f; /**< 深度值最大值，范围0~1*/
};

/** 着色器类型 */
enum ShaderType : uint8_t {
	SHADER_TYPE_VERTEX_SHADER = 0,
	SHADER_TYPE_PIXEL_SHADER,
	SHADER_TYPE_GEOMETRY_SHADER,
	SHADER_TYPE_HULL_SHADER, /**< DX only */
	SHADER_TYPE_TESSELLATION_SHADER, /**< =Domain shader for dx */
	SHADER_TYPE_COMPUTE_SHADER,
	SHADER_TYPE_NUMBER_OF_TYPE
};

/** 程序类型 */
enum ProgramType : uint8_t {
	PROGRAM_TYPE_GRAPHICS,
	PROGRAM_TYPE_COMPUTE
};

enum DescriptorHeapType : uint8_t {
	DESCRIPTOR_HEAP_TYPE_INVALID = 0,
	DESCRIPTOR_HEAP_TYPE_CBV = 1 << 0,
	DESCRIPTOR_HEAP_TYPE_SRV = 1 << 1,
	DESCRIPTOR_HEAP_TYPE_UAV = 1 << 2,
	DESCRIPTOR_HEAP_TYPE_SAMPLER = 1 << 3,
	DESCRIPTOR_HEAP_TYPE_RTV = 1 << 4
};

enum COMMAND_UNIT_TYPE : uint8_t {
	DEFAULT_COMMAND_UNIT = 0,
	COMPUTE_COMMAND_UNIT,
	TRANSFER_COMMAND_UNIT,
	NUMBER_OF_COMMAND_UNIT_TYPE
};

ENUM_LOGICAL_OPERATION(DescriptorHeapType, uint8_t)
#define DESCRIPTOR_HEAP_CBV_SRV_UAV DESCRIPTOR_HEAP_TYPE_CBV | DESCRIPTOR_HEAP_TYPE_SRV | DESCRIPTOR_HEAP_TYPE_UAV

/** 为索引值设置别名，加强类型检查 */
ALIAS_INDEX(uint8_t, VertexAttributeHandle);
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
ALIAS_INDEX(uint64_t, SamplerHandle);
ALIAS_INDEX(uint64_t, ShaderHandle);
ALIAS_INDEX(uint64_t, PipelineHandle);
ALIAS_INDEX(uint64_t, TaskFrame);

template<typename T>
constexpr T InvalidHandleIndex(const T&) {
	return T::InvalidIndex();
}

END_NAME_SPACE
