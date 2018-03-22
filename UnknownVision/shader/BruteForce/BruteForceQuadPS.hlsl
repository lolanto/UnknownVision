#include "../PS_INPUT.hlsli"

struct PSOutput {
  float4 color : SV_Target;
};

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

Texture2D albedo : register (t0);
SamplerState pointSampler : register (s0);

PSOutput main (VSOutput i) {
  PSOutput o;
  o.color = albedo.Sample(pointSampler, i.uv);
  return o;
}
