#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 texcoord : TEXCOORD0;
};

Texture2DArray basicColorArray : register (t0);
SamplerState comSampler : register (s0);

float4 main(VSOutput i) : SV_Target {
  return basicColorArray.Sample(comSampler, float3(i.texcoord, 1));
}
