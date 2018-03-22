#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 normal : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

struct PSOutput {
  float4 color : SV_Target0;
};

TextureCube cubeMap : register (t0);
Texture2D basicColor : register (t1);
SamplerState linearSampler : register (s0);

cbuffer CubeMapData : register (b1) {
  matrix CM_Matrix;
}

static const float3 boxMax = float3(6.35685f, 3.35686f, 6.29802f);
static const float3 boxMin = float3(-6.35685f, -9.35685, -6.41569f);

PSOutput main (VSOutput i) {
  PSOutput o;

  float3 viewDir = normalize(i.worldPos.xyz - GEyePos.xyz);
  float3 reflDir = normalize(reflect(viewDir, i.normal.xyz));
  float3 cm_reflDir = mul(CM_Matrix, float4(reflDir, 0.0f)).xyz;
  float3 cm_tarPos = mul(CM_Matrix, float4(i.worldPos.xyz, 1.0f)).xyz;
  // cm_tarPos = cm_tarPos + cm_reflDir * 12.0f;
  // o.color = cubeMap.Sample(linearSampler, cm_tarPos.xyz);
  // o.color = basicColor.Sample(linearSampler, i.uv);
  // o.color = float4(normalize(cm_tarPos.xyz), 1.0f);
  float3 cm_f = (cm_reflDir.xyz > 0.0f) ? boxMax - cm_tarPos.xyz : boxMin - cm_tarPos.xyz;
  cm_f = cm_f / cm_reflDir.xyz;
  float f = min(min(cm_f.x, cm_f.y ), cm_f.z );
  cm_f = cm_tarPos + cm_reflDir * f;
  o.color = cubeMap.Sample(linearSampler, cm_f);
  return o;
}
