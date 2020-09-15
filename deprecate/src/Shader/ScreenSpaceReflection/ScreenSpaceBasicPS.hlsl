#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float3 csPos : TEXCOORD0;
  float3 csNor : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

struct PSOutput {
  float4 SSNor : SV_TARGET0;
  float4 SSPos : SV_TARGET1;
};

Texture2D basicColor : register (t0);
SamplerState linearSampler : register (s0);

PSOutput main( VSOutput i ) {
  PSOutput o;
  o.SSNor = float4(i.csNor, 0.0f);
  o.SSPos = float4(i.csPos, 1.0f);

  return o;
}