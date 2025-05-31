#include "../Utility/GeneralCamera/GeneralCamera.hlsl.inc"

struct VertexData
{
    float3 Position;
    float2 Texcoord;
    float3 Normal;
};

/* #VERTEX_DECLARATIONS# */

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

/* #ConstantBuffer0# */




VSOutput main(VertexInput input) {
    VertexData resolvedVertexData = GetVertexData(input);
    VSOutput output;
    output.position = 
        mul(GetProjectionMatrix(CameraData),
            mul(GetViewMatrix(CameraData), float4(resolvedVertexData.Position, 1.0f)));
    output.texcoord = resolvedVertexData.Texcoord;
    return output;
}