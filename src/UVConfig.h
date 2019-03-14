#ifndef UV_CONFIG_H
#define UV_CONFIG_H
#include "Utility/TypeRestriction/TypeRestriction.h"
#include "Utility/InfoLog/InfoLog.h"
#include <cstdint>

namespace UnknownVision {
	const uint32_t UV_MAX_RENDER_TARGET = 8; /**< 可绑定的渲染对象的上限 */
	const uint32_t UV_MAX_VERTEX_BUFFER = 16; /**< 可绑定的顶点缓冲的上限 */

	/** 资源记录的类型 */
	enum ResourceRecordType : uint8_t {
		RESOURCE_RECORD_TYPE_READ = 0,
		RESOURCE_RECORD_TYPE_WRITE,
		RESOURCE_RECORD_TYPE_CREATE,
		RESOURCE_RECORD_TYPE_PERMANENT /**< 该资源是持久性的 */
	};

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

	enum ManagerType {
		MT_TEXTURE2D_MANAGER,
		MT_SHADER_MANAGER,
		MT_BUFFER_MANAGER,
		MT_VERTEX_DECLARATION_MANAGER,
		MT_PIPELINE_STATE_MANAGER
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
}
#endif // UV_CONFIG_H
