#include "../PS_INPUT.hlsli"

struct PSOutput {
  float4 worldPos : SV_Target0;
  float4 worldNor : SV_Target1;
  float4 albedo : SV_Target2;
};

struct VSOutput {
  float4 uvPos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 worldNor : TEXCOORD1;
};

Texture2D albedo : register (t0);
SamplerState linearSampler : register (s0);

PSOutput main (VSOutput i) {
  PSOutput o;
  o.albedo = albedo.Sample(linearSampler, float2(i.worldPos.a, i.worldNor.a));
  o.worldPos = i.worldPos;
  o.worldNor = i.worldNor;
  // 标记该点有信息
  o.worldPos.a = 0.5f;
  return o;
}
