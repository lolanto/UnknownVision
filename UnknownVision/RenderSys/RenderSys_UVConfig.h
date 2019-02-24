#ifndef RENDER_SYS_UV_CONFIG_H
#define RENDER_SYS_UV_CONFIG_H
#include "../Utility/TypeRestriction/TypeRestriction.h"
#include "../UVConfig.h"

namespace UnknownVision {

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

	struct SamplerDesc {

	};

	ALIAS_INDEX(uint32_t, SlotIdx);

} // namespace UnknownVision

#endif // RENDER_SYS_UV_CONFIG_H
