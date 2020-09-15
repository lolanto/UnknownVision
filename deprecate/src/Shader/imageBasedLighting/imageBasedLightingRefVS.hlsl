#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float3 worldPos : TEXCOORD1;
  float3 worldNor : TEXCOORD2;
  float2 uv : TEXCOORD3;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.worldPos = mul(GModelMatrix, float4(i.position, 1.0f)).xyz;
  o.worldNor = mul(GModelMatrix, float4(i.normal, 0.0f)).xyz;
  o.pos =
  mul(GProjectMatrix,
    mul(GViewMatrix, float4(o.worldPos, 1.0f)));
  o.uv = i.texcoord;
  return o;
}