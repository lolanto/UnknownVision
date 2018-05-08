#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float3 csNor : TEXCOORD0;
  float3 csPos : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.csNor = mul(GViewMatrix,
    mul(GModelMatrix, float4(i.normal, 0.0f))).xyz;
  o.csPos = mul(GViewMatrix,
    mul(GModelMatrix, float4(i.position, 1.0f))).xyz;
  o.pos = mul(GProjectMatrix, float4(o.csPos, 1.0f));
  o.uv = i.texcoord;
  return o;
}