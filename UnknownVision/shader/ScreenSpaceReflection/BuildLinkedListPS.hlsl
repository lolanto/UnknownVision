#include "../PS_INPUT.hlsli"
#include "LinkedList.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD2;
};

Texture2D basicColor : register (t0);
SamplerState linearSampler : register (s0);

void main( VSOutput i ) {

  BuildLinkedList(i.pos.xy, basicColor.Sample(linearSampler, i.uv), i.pos.z);

  return ;
}
