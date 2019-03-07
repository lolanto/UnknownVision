#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 worldNor : TEXCOORD1;
  float4 worldRef : TEXCOORD2;
};

struct PSOutput {
  float4 worldPos : SV_Target0;
  float4 worldNor : SV_Target1;
  float4 worldRef : SV_Target2;
  float4 albedo : SV_Target3;
};

Texture2D Albedo : register (t0);
SamplerState linearSampler : register (s0);

PSOutput main (VSOutput i) {
  PSOutput o;
  o.worldPos = i.worldPos;
  o.worldNor = float4(i.worldNor.xyz, 0.0f);
  o.worldRef = float4(i.worldRef.xyz, 0.0f);
  o.albedo = Albedo.Sample(linearSampler, float2(i.worldNor.w, i.worldRef.w));
  return o;
}
