#include "../Utility/SimpleLight/SimpleLight.hlsl.inc"
#include "../Utility/GeneralCamera/GeneralCamera.hlsl.inc"
#include "PBRHeader.hlsl.inc"

struct PSInput {
    float4 position : SV_POSITION;
    float4 worldPosition : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 bitangent : TEXCOORD3;
    float2 texcoord : TEXCOORD4;
};

cbuffer LightDataBuffer : register(b0) {
    LightDataBufferStructure PointLightData[4];
}

cbuffer CameraDataBuffer : register(b1) {
    GeneralCameraDataStructure cameraData;
}

cbuffer ControlPanel : register(b2) {
    float4 Albedo;
    float Roughness;
    float Metallic;
    float NormalPower;
}

Texture2D BaseColor : register(t0);
Texture2D Normal : register(t1);
Texture2D ARM : register(t2);
TextureCube EnvMap : register(t3);
TextureCube EnvMap_C : register(t4);

SamplerState LinearSampler : register(s0);

float3 NormalCalculation(float2 uv, float3 X, float3 Y, float3 Z) {
    float3 offset = Normal.Sample(LinearSampler, uv).rgb;
    offset = pow(offset, 1.0f / NormalPower);
    offset.z = sqrt(1.0f - dot(offset.xy, offset.xy));
    return normalize(X * offset.x + Y * offset.y + Z * offset.z);
}

static const float N = 32;

float3 Fibonaccilattice(float n) {
    float3 output = float3(0.0f, 0.0f, 0.0f);
    output.y = (2 * n - 1.0f) / N - 1.0f;
    float t = sqrt(1.0f - output.y * output.y);
    float k = 2.0 * PI * n * 0.618f;
    output.x = t * cos(k);
    output.z = t * sin(k);
    output.y = abs(output.y);
    return output;
}

float4 main(PSInput input) : SV_Target {
    float4 output = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 albedo = Albedo.rgb;
    float3 o2v = GetCameraPosition(cameraData) - input.worldPosition.xyz;
    float3 normal = NormalCalculation(input.texcoord, input.tangent, input.bitangent, input.normal);
    
    o2v = normalize(o2v);
    // for(int i = 0; i < 4; ++i) {
    //     float3 o2l = PointLight_Position(PointLightData[i]) - input.worldPosition.xyz;
    //     o2l = normalize(o2l);
    //     float NdotL = saturate(dot(normal, o2l));
    //     float3 specular = float3(0.0f, 0.0f, 0.0f);
    //     float3 Kd = float3(0.0f, 0.0f, 0.0f);
    //     PBR(o2l, o2v, normal, albedo, Roughness, Metallic, specular, Kd);
    //     float3 diffuse = Kd * albedo / PI;
    //     output.rgb += (specular + diffuse) * PointLight_RadianceMulPower(PointLightData[i]) * NdotL;
    // }
    output.rgb += EnvMap_C.Sample(LinearSampler, normal) * albedo;
    return output;
}
