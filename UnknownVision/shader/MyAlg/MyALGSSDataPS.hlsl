// 渲染屏幕空间信息，构造G-Buffer
#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

struct PSOutput {
  float4 wPos : SV_Target0;
  float4 wNor : SV_Target1;
  uint ID : SV_Target2;
};

Texture2D IDTex : register(t0);
SamplerState pointSampler : register(s0);

PSOutput main(VSOutput i, uint PrimitiveID : SV_PrimitiveID) {
  PSOutput o;
  o.wPos = i.wPos;
  o.wNor = i.wNor;
  // o.ID = IDTex.Sample(pointSampler, i.uv).r * 255;
  o.ID = PrimitiveID;
  return o;
}
