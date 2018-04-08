#include "../PS_INPUT.hlsli"
#include "LinkedList.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float3 csPos : TEXCOORD0;
  float3 csNor : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

struct PSOutput {
  float4 SSBasicColor : SV_TARGET0;
};

Texture2D basicColor : register (t0);
SamplerState linearSampler : register (s0);

PSOutput main( VSOutput i ) {
  PSOutput o;
  o.SSBasicColor = basicColor.Sample(linearSampler, i.uv);

  BuildLinkedList(i.pos.xy, o.SSBasicColor, i.pos.z);

  return o;
}
