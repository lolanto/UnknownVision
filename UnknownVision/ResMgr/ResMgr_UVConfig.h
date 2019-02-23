#ifndef RESOURCE_MANAGER_UV_CONFIG_H
#define RESOURCE_MANAGER_UV_CONFIG_H
#include "../UVConfig.h"
#include "../Utility/TypeRestriction/TypeRestriction.h"

namespace UnknownVision {
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
	ALIAS_INDEX(int32_t, PipelineStateIdx);

	using TextureFlagCombination = uint32_t;
	using BufferFlagCombination = uint32_t;
}

#endif // RESOURCE_MANAGER_UV_CONFIG_H
