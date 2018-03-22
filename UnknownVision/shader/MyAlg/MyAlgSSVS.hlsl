#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 worldNor : TEXCOORD1;
  float4 worldRef : TEXCOORD2;
};

VSOutput main (a2v i) {
  VSOutput o;
  o.worldPos = mul(GModelMatrix, float4(i.position, 1.0f));
  o.worldNor = normalize(mul(GModelMatrix, float4(i.normal, 0.0f)));
  o.worldRef.xyz = normalize(reflect(o.worldPos.xyz - GEyePos.xyz, o.worldNor.xyz));

  // 藏起UV
  o.worldNor.w = i.texcoord.x;
  o.worldRef.w = i.texcoord.y;
  o.pos = 
  mul(GProjectMatrix,
    mul(GViewMatrix, o.worldPos));
  // hemisphere mapping
  // o.pos = mul(GViewMatrix, o.worldPos);
  // float len = length(o.pos.xyz);
  // o.pos.xyz = normalize(o.pos.xyz);
  // o.pos.z += 1.0f;
  // o.pos.xy = o.pos.xy / o.pos.z;
  // o.pos.z = (len - GCameraParam.x) / (GCameraParam.w);
  // o.pos.w = 1.0f; 
  return o;
}
