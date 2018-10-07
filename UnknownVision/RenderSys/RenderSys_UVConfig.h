#pragma once
#include "../UVConfig.h"

enum Shader_Type {
	Shader_Type_Vertex_Shader,
	Shader_Type_Pixel_Shader,
	Shader_Type_Geometry_Shader,
	Shader_Type_Compute_Shader
};

enum Canvas_Usage {
	Canvas_Usage_Read = 2,
	Canvas_Usage_Write = 4
};