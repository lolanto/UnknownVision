#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
};

Texture2D result : register(t0);
SamplerState pointSampler : register (s0);

float4 main(VSOutput i) : SV_Target {
  return result.Sample(pointSampler, i.pos.xy / 960.0f);
}
