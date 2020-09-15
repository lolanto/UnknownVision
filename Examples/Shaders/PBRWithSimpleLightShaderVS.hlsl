#include "../Utility/GeneralCamera/GeneralCamera.hlsl.inc"
struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 worldPosition : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 bitangent : TEXCOORD3;
    float2 texcoord : TEXCOORD4;
};

cbuffer ObjectTransformsBuffer : register(b0) {
    float4x4 modelMat;
}

cbuffer CameraDataBuffer : register(b1) {
    GeneralCameraDataStructure cameraData;
}

PSInput main(VSInput input) {
    PSInput output;
    output.worldPosition = mul(modelMat, float4(input.position.xyz, 1.0f));
    output.position = mul(GetProjectionMatrix(cameraData),
        mul(GetViewMatrix(cameraData), output.worldPosition)
    );
    output.normal = mul(modelMat, float4(input.normal.xyz, 0.0f));
    output.tangent = mul(modelMat, float4(input.tangent.xyz, 0.0f));
    output.bitangent = cross(output.normal, output.tangent);
    output.texcoord = input.texcoord;
    return output;
}
