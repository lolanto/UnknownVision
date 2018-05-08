#include "../VS_INPUT.hlsli"

cbuffer ShadowMatrixs : register(b0) {
  float4 posAndInside;
  float4 color;
  float4 orientAndOutside;
  float4x4 viewMatrix;
  float4x4 projMatrix;
}

struct VSOutput {
  float4 pos : SV_POSITION;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.pos = 
    mul(projMatrix,
      mul(viewMatrix,
        mul(GModelMatrix, float4(i.position, 1.0f))));
  return o;
}
