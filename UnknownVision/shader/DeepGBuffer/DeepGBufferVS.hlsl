#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 texcoord : TEXCOORD0;
};

VSOutput main (a2v i) {
  VSOutput o;
  o.pos = 
  mul(GProjectMatrix,
    mul(GViewMatrix,
      mul(GModelMatrix, float4(i.position, 1.0f))));
  o.texcoord = i.texcoord;
  return o;
}