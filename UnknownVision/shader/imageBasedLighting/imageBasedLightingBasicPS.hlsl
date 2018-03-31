#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
};

struct PSOutput {
  float4 color : SV_Target0;
};

Texture2D basicColor : register (t0);
SamplerState linearSampler : register (s0);

PSOutput main (VSOutput i) {
  PSOutput o;
  o.color = basicColor.SampleLevel(linearSampler, i.uv * 0.75 + 0.125, 0);
  return o;
}