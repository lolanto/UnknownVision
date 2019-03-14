#ifndef UV_RENDER_GRAPH_RAW_H
#define UV_RENDER_GRAPH_RAW_H

#include "RenderSystemConfig.h"

#include <string>
#include <vector>

namespace UnknownVision {

	struct ResourceRawOperation {
		std::string name;
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
