#include "../PS_INPUT.hlsli"

struct GSOutput {
  float4 position : SV_POSITION;
  float2 texcoord : TEXCOORD0;
  uint slice : SV_RenderTargetArrayIndex;
};

Texture2D basicColor : register(t0);
SamplerState comSampler : register(s0);

float4 main( GSOutput i ) : SV_TARGET
{
  return basicColor.Sample(comSampler, i.texcoord);
}