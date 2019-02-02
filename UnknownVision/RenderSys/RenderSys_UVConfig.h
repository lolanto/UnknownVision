#ifndef RENDER_SYS_UV_CONFIG_H
#define RENDER_SYS_UV_CONFIG_H
#include "../Utility/TypeRestriction/TypeRestriction.h"
#include "../UVConfig.h"

namespace UnknownVision {
	const uint32_t UV_MAX_RENDER_TARGET = 8; /**< 可绑定的渲染对象的上限 */
	const uint32_t UV_MAX_VERTEX_BUFFER = 16; /**< 可绑定的顶点缓冲的上限 */

	/** 视口设置描述对象 */
	struct ViewPortDesc {
		float topLeftX = 0.0f; /**< 视口的左上角横坐标，单位为像素 */
		float topLeftY = 0.0f; /**< 视口的左上角纵坐标，单位为像素 */
		float width = 0.0f; /**< 视口的宽度，单位为像素 */
		float height = 0.0f; /**< 视口的高度，单位为像素 */
		float minDepth = 0.0f; /**< 深度值最小值，范围0~1 */
		float maxDepth = 1.0f; /**< 深度值最大值，范围0~1*/
	};

	/** 标记管线不同部分的枚举值
	* 凡涉及对特定管线部分进行状态设置的
	* 函数均需要提供该枚举值，说明需要设置的部分
	*/
	enum PipelineStage {
		PS_InputProcess, /**< 管线输入阶段 */
		PS_VertexProcess, /**< 顶点处理阶段 */
		PS_GeometryProcess, /**< 几何处理阶段 */
		PS_TesselationProcess, /**< 细分处理阶段 */
		PS_RasterizationProcess, /**< 光栅处理阶段 */
		PS_PixelProcess, /**< 像素处理阶段 */
		PS_OutpuProcess /**< 管线输出处理阶段 */
	};
	/** 图元类型的枚举值，与光栅化相关
	*/
	enum Primitive {
		PRI_Point, /**< 点图元 */
		PRI_TriangleList /**< 三角形列表图元 */
	};

	struct SamplerDesc {

	};

	ALIAS_INDEX(uint32_t, SlotIdx);

} // namespace UnknownVision

#endif // RENDER_SYS_UV_CONFIG_H
