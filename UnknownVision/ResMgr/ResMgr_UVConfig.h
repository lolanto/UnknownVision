#ifndef RESOURCE_MANAGER_UV_CONFIG_H
#define RESOURCE_MANAGER_UV_CONFIG_H
#include "../UVConfig.h"

namespace UnknownVision {
	enum ShaderType {
		ST_Vertex_Shader,
		ST_Pixel_Shader,
		ST_Geometry_Shader,
		ST_Compute_Shader
	};

	enum BufferFlag : uint32_t {
		BF_INVALID = 0,
		BF_CPU_WRITE = 0x00000001U, // CPU能够写
		BF_CPU_READ = 0x00000002U, // CPU能够读
		BF_VERTEX_BUFFER = 0x00000004U,
		BF_CONSTANT_BUFFER = 0x00000008U
	};

	enum TextureElementType {
		TET_INVALID = 0,
		UNORM_R8G8B8,
		UNORM_R8G8B8A8,
		UNORM_R16G16B16,
		UNORM_R16G16B16A16,
		UNORM_R32G32B32,
		UNORM_R32G32B32A32,
		UNORM_D24_UINT_S8
	};

	enum TextureFlag : uint32_t {
		TF_INVALID = 0,
		TF_WRITE = 0x00000001U,
		TF_ONLY_READ = 0x00000002U,
		TF_DEPTH_STENCIL = 0x0000004U
	};
}

#endif // RESOURCE_MANAGER_UV_CONFIG_H
