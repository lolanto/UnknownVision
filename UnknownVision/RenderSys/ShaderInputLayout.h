#ifndef SHADER_INPUT_LAYOUT_H
#define SHADER_INPUT_LAYOUT_H

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


} // namespace UnknownVision

#endif // SHADER_INPUT_LAYOUT_H
