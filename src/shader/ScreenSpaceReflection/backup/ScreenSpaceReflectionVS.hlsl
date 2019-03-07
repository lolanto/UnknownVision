#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 normalAndLinearZ : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.pos = 
  mul(GViewMatrix,
    mul(GModelMatrix, float4(i.position, 1.0f)));
  o.normalAndLinearZ.w = o.pos.z;
  o.pos = mul(GProjectMatrix, o.pos);

  o.normalAndLinearZ.xyz = 
  mul(GViewMatrix,
    mul(GModelMatrix, float4(i.normal, 0.0f))).xyz;
  o.normalAndLinearZ.xyz = normalize(o.normalAndLinearZ.xyz);
  o.uv = i.texcoord;
  return o;
}