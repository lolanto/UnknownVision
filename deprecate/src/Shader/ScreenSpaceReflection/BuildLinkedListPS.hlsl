#include "../PS_INPUT.hlsli"
#include "LinkedList.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float3 csNor : TEXCOORD0;
  float3 csPos : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

Texture2D basicColor : register (t0);
SamplerState linearSampler : register (s0);

void main( VSOutput i ) {

  BuildLinkedList(i.pos.xy, basicColor.Sample(linearSampler, i.uv), 
    i.pos.w, i.csNor, i.csPos);

  return ;
}
