#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.pos = 
    mul(GModelMatrix, float4(i.position, 1.0f));
  o.uv = i.texcoord;
  return o;
}