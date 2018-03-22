#include "../PS_INPUT.hlsli"

struct v2p {
  float4 position : SV_POSITION;
  float4 eye : TEXCOORD0;
  float2 texcoord : TEXCOORD1;
};

SamplerState comSampler : register (s0);

TextureCube cubeMap : register (t0);

float4 main (v2p i) : SV_TARGET {
  float3 t = float3(i.texcoord, 1.0f);
  t.xy = t.xy * 2 - 1;
  t.y = -t.y;
  t = mul(GViewMatrix, float4(t, 1.0f)).xyz;
  return cubeMap.Sample(comSampler, t.xyz);
}
