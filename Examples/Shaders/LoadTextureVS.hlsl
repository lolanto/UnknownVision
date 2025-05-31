#include "../Utility/GeneralCamera/GeneralCamera.hlsl.inc"
struct VSInput {
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

/* #ConstantBuffer0# */

struct VertexData
{
    float3 Position;
    float2 Texcoord;
    float3 Normal;
}


VSOutput main(VSInput input) {
    VSOutput output;
    output.position = 
        mul(GetProjectionMatrix(CameraData),
            mul(GetViewMatrix(CameraData), float4(input.position, 1.0f)));
    output.texcoord = input.texcoord;
    return output;
}