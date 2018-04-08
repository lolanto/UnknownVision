#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 normalAndLinearZ : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

struct PSOutput {
  float4 color : SV_TARGET0;
  float4 normalAndLinearZ : SV_TARGET1;
};

Texture2D basicColor : register (t0);
SamplerState linearSampler : register (s0);

PSOutput main( VSOutput i ) {
  PSOutput o;
  o.color = basicColor.Sample(linearSampler, i.uv);
  o.normalAndLinearZ = i.normalAndLinearZ;
  return o;
}
