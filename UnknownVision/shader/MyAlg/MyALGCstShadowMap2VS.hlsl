#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.pos = 
    mul(GModelMatrix, float4(i.position, 1.0f));
  o.wPos = o.pos;
  o.wNor =
    mul(GModelMatrix, float4(i.normal, 1.0f));
  return o;
}