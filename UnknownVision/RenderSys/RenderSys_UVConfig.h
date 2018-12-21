#pragma once
#include "../UVConfig.h"

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
	Buffer_Type_Depth_Stencil_Buffer
};

enum Canvas_Usage {
	Canvas_Usage_Read = 2,
	Canvas_Usage_Write = 4
};