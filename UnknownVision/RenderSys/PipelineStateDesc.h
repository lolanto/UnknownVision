#ifndef PIPELINE_STATE_DESC_H
#define PIPELINE_STATE_DESC_H

namespace UnknownVision {
	enum VertexAttributeDataType : uint8_t {
		VADT_FLOAT1 = 0,
		VADT_FLOAT2,
		VADT_FLOAT3,
		VADT_FLOAT4
	};
	struct SubVertexAttributeLayoutDesc {
		const char* semantic = nullptr;
		uint8_t index = 0;
		VertexAttributeDataType dataType;
		uint8_t bufIdx = 0;
		uint8_t byteOffset = 0;
	};

	struct ViewPortDesc {
		float topLeftX = 0.0f;
		float topLeftY = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float minDepth = 0.0f;
		float maxDepth = 1.0f;
	};

} // namespace UnknownVision

#endif // PIPELINE_STATE_DESC_H
