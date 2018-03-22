#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 normal : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

VSOutput main (a2v i) {
  VSOutput o;
  o.worldPos = mul(GModelMatrix, float4(i.position, 1.0f));
  o.normal = mul(GModelMatrix, float4(i.normal, 0.0f));
  o.pos =
  mul(GProjectMatrix,
    mul(GViewMatrix, o.worldPos));
  o.uv = i.texcoord;
  return o;
}
