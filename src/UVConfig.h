﻿#pragma once
#include "Utility/TypeRestriction/TypeRestriction.h"
#include <cstdint>

#define allow_logical_operation
#define PROJECT_NAME_SPACE UnknownVision
#define BEG_NAME_SPACE namespace PROJECT_NAME_SPACE {
#define END_NAME_SPACE }

#define thread_safe
#define thread_safe_const const

BEG_NAME_SPACE
	const uint32_t UV_MAX_RENDER_TARGET = 8; /**< 可绑定的渲染对象的上限 */
	const uint32_t UV_MAX_VERTEX_BUFFER = 16; /**< 可绑定的顶点缓冲的上限 */

	enum PassType : uint8_t {
		PASS_TYPE_GRAPHIC = 0,
		PASS_TYPE_COMPUTE = 1
	};

	/** 资源记录的类型 */
	enum ResourceRecordType : uint8_t {
		RESOURCE_RECORD_TYPE_READ = 0x01U,
		RESOURCE_RECORD_TYPE_WRITE = 0x02U,
		RESOURCE_RECORD_TYPE_CREATE = 0x04U,
		RESOURCE_RECORD_TYPE_PERMANENT = 0x08U /**< 该资源是持久性的，与create相对 */
	};

	/** 资源大致类型 */
	enum ResourceType : uint8_t {
		RESOURCE_TYPE_BUFFER = 0x01U,
		RESOURCE_TYPE_TEXTURE1D = 0x02U,
		RESOURCE_TYPE_TEXTURE2D = 0x04U,
		RESOURCE_TYPE_TEXTURE3D = 0x08U,
		RESOURCE_TYPE_TRANSIENT = 0x80U /**< 该资源是临时存在的 */
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

	enum ResourceState : uint8_t {
		RESOURCE_STATE_INVALID = 0,
		RESOURCE_STATE_CONSTANT_BUFFER,
		RESOURCE_STATE_VERTEX_BUFFER,
		RESOURCE_STATE_GENERIC_READ, /** 这是DX12创建Uploadheap上的资源使用的，需要考虑如何去除，让它更通用 */
		RESOURCE_STATE_COPY_DEST,
		RESOURCE_STATE_COPY_SRC
	};

#define CanBe(x, X) inline bool canBe##x() const { return usage & RESOURCE_USAGE_##X; }
#define IsFlag(x, X) inline bool is##x() const { return flag & RESOURCE_FLAG_##X; }
#define IsInState(x, X) inline bool isInStateOf##x() const { return state == RESOURCE_STATE_##X; }
	struct ResourceStatus {
		ResourceUsage usage = RESOURCE_USAGE_INVALID;
		ResourceState state = RESOURCE_STATE_INVALID;
		ResourceFlag flag = RESOURCE_FLAG_INVALID;

		ResourceStatus() = default;
		ResourceStatus(ResourceUsage usage, ResourceState state, ResourceFlag flag)
			:usage(usage), state(state), flag(flag) {}
		/** helper functions */
		inline bool isInvalid() const { 
			return usage == RESOURCE_USAGE_INVALID ||
				state == RESOURCE_STATE_INVALID ||
				flag == RESOURCE_FLAG_INVALID; }
		IsInState(ConstantBuffer, CONSTANT_BUFFER)
		IsInState(VertexBuffer, VERTEX_BUFFER)
		IsInState(CopyDest, COPY_DEST)
		IsInState(CopySrc, COPY_SRC)
		IsFlag(Stably, STABLY)
		IsFlag(Once, ONCE)
		IsFlag(Frequently, FREQUENTLY)
		IsFlag(ReadBack, READ_BACK)
		CanBe(VertexBuffer, VERTEX_BUFFER)
		CanBe(IndexBuffer, INDEX_BUFFER)
		CanBe(ConstantBuffer, CONSTANT_BUFFER)
	};
#undef IsInState
#undef IsFlag
#undef CanBe

	enum DataFormatType : uint8_t {
		/** 以下格式可以等价为float1, float2, float3以及float4 */
		DFT_R32_FLOAT = 0,
		DFT_R32G32_FLOAT,
		DFT_R32G32B32_FLOAT,
		DFT_R32G32B32A32_FLOAT,
		DFT_R8G8B8A8_UNORM, /**< 常用于描述渲染对象元素的格式 */
		/**/
		DFT_D24_UNORM_S8_UINT, /**< 常用于描述深度模板缓存元素的格式 */

		DFT_INVALID = UINT8_MAX /**< 无效的默认值 */
	};

	struct SubVertexAttributeDesc {
		const char* semantic = nullptr;
		uint8_t index = 0;
		DataFormatType dataType;
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

	END_NAME_SPACE
