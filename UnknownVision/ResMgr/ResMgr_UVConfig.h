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
		BF_INVALID = 0
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
		WRITE = 0x00000001U,
		READ = 0x00000002U,
		DEPTH_STENCIL = 0x0000006U
	};
}

#endif // RESOURCE_MANAGER_UV_CONFIG_H
