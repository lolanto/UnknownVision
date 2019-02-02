#ifndef RESOURCE_MANAGER_UV_CONFIG_H
#define RESOURCE_MANAGER_UV_CONFIG_H
#include "../UVConfig.h"
#include "../Utility/TypeRestriction/TypeRestriction.h"

namespace UnknownVision {
	enum ShaderType {
		ST_Vertex_Shader,
		ST_Pixel_Shader,
		ST_Geometry_Shader,
		ST_Compute_Shader
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

	enum TextureElementType : uint32_t {
		TET_INVALID = 0,
		UNORM_R8G8B8,
		UNORM_R8G8B8A8,
		UNORM_R16G16B16,
		UNORM_R16G16B16A16,
		UNORM_R32G32B32,
		UNORM_R32G32B32A32,
		// 深度模板测试缓存专用的数据类型
		UNORM_D24_UINT_S8
	};

	enum TextureFlag : uint32_t {
		TF_INVALID = 0x00000000U,
		TF_READ_BY_GPU = 0x00000001U,
		TF_WRITE_BY_GPU = 0x00000002U,
		TF_READ_BY_CPU = 0x00000004U,
		TF_WRITE_BY_CPU = 0x00000008U
	};

	ALIAS_INDEX(int32_t, Texture2DIdx);
	ALIAS_INDEX(int32_t, RenderTargetIdx);
	ALIAS_INDEX(int32_t, DepthStencilIdx);
	ALIAS_INDEX(int32_t, ShaderIdx);
	ALIAS_INDEX(int32_t, BufferIdx);
	ALIAS_INDEX(int32_t, VertexDeclarationIdx);

	//using Texture2DIdx = int32_t;
	//using RenderTargetIdx = int32_t;
	//using DepthStencilIdx = int32_t;
	//using ShaderIdx = int32_t;
	//using BufferIdx = int32_t;
	//using VertexDeclarationIdx = int32_t;
	using TextureFlagCombination = uint32_t;
	using BufferFlagCombination = uint32_t;
}

#endif // RESOURCE_MANAGER_UV_CONFIG_H
