#ifndef RENDER_SYS_UV_CONFIG_H
#define RENDER_SYS_UV_CONFIG_H
#include "../UVConfig.h"

namespace UnknownVision {
	enum Shader_Type {
		Shader_Type_Vertex_Shader,
		Shader_Type_Pixel_Shader,
		Shader_Type_Geometry_Shader,
		Shader_Type_Compute_Shader
	};

	enum Buffer_Type {
		Buffer_Type_Vertex_Buffer,
		Buffer_Type_Index_Buffer,
		Buffer_Type_Depth_Buffer,
		Buffer_Type_Stencil_Buffer,
		Buffer_Type_Depth_Stencil_Buffer,
		Buffer_Type_Render_Target,
		Buffer_Type_Texture
	};

	enum TextureElementType {
		TET_INVALID = 0,
		NORM_FLOAT_RGB,
		NORM_FLOAT_RGBA
	};

	enum TextureFlag : uint32_t {
		TF_INVALID = 0,
		WRITE = 0x00000001,
		READ = 0x00000002,
	};
}

#endif // RENDER_SYS_UV_CONFIG_H
