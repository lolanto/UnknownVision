#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float4 wTan : TEXCOORD2;
  float4 wBin : TEXCOORD3;
  float2 uv : TEXCOORD4;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.pos = 
    mul(GModelMatrix, float4(i.position, 1.0f));
  o.wPos = o.pos;
  o.wNor =
    mul(GModelMatrix, float4(i.normal, 0.0f));
  o.wTan = 
    mul(GModelMatrix, float4(i.tangent, 0.0f));
  o.wBin = float4(cross(o.wNor.xyz, o.wTan.xyz), 0.0f);
  o.uv = i.texcoord;
  return o;
}