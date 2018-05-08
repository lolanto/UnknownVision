// 渲染屏幕空间信息，构造G-Buffer
#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 o2wMatrixR1 : TEXCOORD0;
  float4 o2wMatrixR2 : TEXCOORD1;
  float4 o2wMatrixR3 : TEXCOORD2;
  float2 uv : TEXCOORD3;
};

VSOutput main(a2v i) {
  VSOutput o;
  float3 wPos = mul(GModelMatrix, float4(i.position, 1.0f)).xyz;
  float3 wNor = mul(GModelMatrix, float4(i.normal, 0.0f)).xyz;
  float3 wTan = mul(GModelMatrix, float4(i.tangent, 0.0f)).xyz;
  float3 wBin = cross(wNor, wTan);
  o.pos = mul(GProjectMatrix,
    mul(GViewMatrix, float4(wPos, 1.0f)));
  o.uv = i.texcoord;

  o.o2wMatrixR1 = float4(wTan.x, wBin.x, wNor.x, wPos.x);
  o.o2wMatrixR2 = float4(wTan.y, wBin.y, wNor.y, wPos.y);
  o.o2wMatrixR3 = float4(wTan.z, wBin.z, wNor.z, wPos.z);
  return o;
}
