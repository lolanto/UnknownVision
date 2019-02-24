#ifndef UV_CONFIG_H
#define UV_CONFIG_H
#include "Utility/InfoLog/InfoLog.h"
#include <cstdint>

namespace UnknownVision {
	const uint32_t UV_MAX_RENDER_TARGET = 8; /**< 可绑定的渲染对象的上限 */
	const uint32_t UV_MAX_VERTEX_BUFFER = 16; /**< 可绑定的顶点缓冲的上限 */

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
}
#endif // UV_CONFIG_H
