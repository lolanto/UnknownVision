#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 texcoord : TEXCOORD0;
};

Texture2D ref : register (t0);
Texture2D ori : register (t1);
SamplerState pointSampler : register (s0);

float4 main(VSOutput i) : SV_Target {
  // return ref.Sample(pointSampler, i.texcoord) * 0.3 + ori.Sample(pointSampler, i.texcoord) * 0.7;
  return ref.Sample(pointSampler, i.texcoord);
  // return ori.Sample(pointSampler, i.texcoord);
}
