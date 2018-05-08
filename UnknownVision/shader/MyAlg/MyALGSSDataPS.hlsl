// 渲染屏幕空间信息，构造G-Buffer
#include "../PS_INPUT.hlsli"
struct VSOutput {
  float4 pos : SV_POSITION;
  float4 o2wMatrixR1 : TEXCOORD0;
  float4 o2wMatrixR2 : TEXCOORD1;
  float4 o2wMatrixR3 : TEXCOORD2;
  float2 uv : TEXCOORD3;
};

struct PSOutput {
  float4 wPos : SV_Target0;
  float4 wNor : SV_Target1;
  float4 wNorModify : SV_Target2;
  float4 wRef : SV_Target3;
  uint ID : SV_Target4;
  float2 uv : SV_Target5;
};

Texture2D NormalMap : register(t0);
SamplerState pointSampler : register(s0);

PSOutput main(VSOutput i, uint PrimitiveID : SV_PrimitiveID) {
  PSOutput o;

  float3 normal = NormalMap.Sample(pointSampler, i.uv).xyz;
  normal.xy = (normal.xy * 2.0f - 1.0f);

  const float bias = 1.0f;
  normal.xy *= bias;
  normal.z = sqrt(1.0f - dot(normal.xy, normal.xy));

  o.wNorModify = float4(dot(i.o2wMatrixR1.xyz, normal),
    dot(i.o2wMatrixR2.xyz, normal),
    dot(i.o2wMatrixR3.xyz, normal), 0.0f);

  o.ID = PrimitiveID;
  o.wPos = float4(i.o2wMatrixR1.w, i.o2wMatrixR2.w, i.o2wMatrixR3.w, 1.0f);

  o.wNor = float4(i.o2wMatrixR1.z, i.o2wMatrixR2.z, i.o2wMatrixR3.z, 0.0f);

  float3 viewDir = normalize(o.wPos.xyz - GEyePos.xyz);
  o.wRef = float4(reflect(viewDir, o.wNor.xyz), 0.0f);

  o.uv = i.uv;
  return o;
}
