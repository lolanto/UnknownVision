#ifndef UV_CONFIG_H
#define UV_CONFIG_H
#include "Utility/InfoLog/InfoLog.h"

namespace UnknownVision {
	enum VertexAttributeDataType : uint8_t {
		VADT_FLOAT1 = 0,
		VADT_FLOAT2,
		VADT_FLOAT3,
		VADT_FLOAT4
	};
	struct SubVertexAttributeDesc {
		const char* semantic = nullptr;
		uint8_t index = 0;
		VertexAttributeDataType dataType;
		uint8_t bufIdx = 0;
		uint8_t byteOffset = 0;
	};

	enum ManagerType {
		MT_TEXTURE2D_MANAGER,
		MT_SHADER_MANAGER,
		MT_BUFFER_MANAGER,
		MT_VERTEX_DECLARATION_MANAGER
	};

	enum API_TYPE {
		DirectX11_0 = 0
	};
}
#endif // UV_CONFIG_H
