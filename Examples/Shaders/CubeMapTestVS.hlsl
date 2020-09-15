#include "../Utility/GeneralCamera/GeneralCamera.hlsl.inc"
struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;
};

cbuffer CameraDataBuffer : register(b0) {
    GeneralCameraDataStructure CameraData;
}

PSInput main(VSInput input) {
    PSInput output;
    output.position = mul(GetProjectionMatrix(CameraData),
        mul(GetViewMatrix(CameraData), float4(input.position, 1.0f)));
    output.texcoord = input.normal;
    return output;
}
