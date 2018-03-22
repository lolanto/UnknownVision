#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 worldNor : TEXCOORD1;
  float4 worldRef : TEXCOORD2;
  float2 uv : TEXCOORD3;
};

VSOutput main (a2v i) {
  VSOutput o;
  o.worldPos = mul (GModelMatrix, float4(i.position, 1.0f));
  o.worldNor = normalize(mul (GModelMatrix, float4(i.normal, 0.0f)));
  o.worldRef = float4(
    normalize(reflect(o.worldPos.xyz - GEyePos.xyz, o.worldNor.xyz)), 0.0f);
  o.uv = i.texcoord;
  o.pos =
  mul(GProjectMatrix,
    mul(GViewMatrix, o.worldPos));
  return o;
}
