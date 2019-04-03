#ifndef UV_RENDER_GRAPH_RAW_H
#define UV_RENDER_GRAPH_RAW_H

#include "RenderSystemConfig.h"

#include <string>
#include <vector>

namespace UnknownVision {

	struct ResourceRawOperation {
		ResourceRawOperation(const char* name, ResourceType type,
			ResourceUsage usage = RESOURCE_USAGE_AUTO,
			uint32_t width = 0, uint32_t height = 0, uint32_t depth = 0,
			DataFormatType format = DFT_INVALID)
			: name(name), type(type), usage(usage), width(width), height(height),
			depth(depth), format(format) {}

		std::string name; /**< 资源名称 */
		ResourceType type; /**< 资源类型，缓冲/纹理 */
		ResourceUsage usage; /**< 资源使用方式 */
		uint32_t width, height, depth; /**< 资源的大小 */
		DataFormatType format; /**< 资源的数据格式 */
		std::vector<uint8_t> initData; /**< 可选！资源初始的字节数据 */
	};

	struct PassRawData
	{
		std::string name;
		std::vector<ResourceRawOperation> writeRecords;
		std::vector<ResourceRawOperation> readRecords;
		std::vector<ResourceRawOperation> createRecords;
	};
}

#endif // UV_RENDER_GRAPH_RAW_H
