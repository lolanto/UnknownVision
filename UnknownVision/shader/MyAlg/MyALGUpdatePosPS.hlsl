// 更新场景中所有点的位置，法线，切线以及副切线资料
#include "../PS_INPUT.hlsli"

Texture2D BasicColor : register(t0);
SamplerState linearSampler : register(s0);

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNormal : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

struct PSOutput {
  float4 wPos : SV_Target0;
  float4 wNormal : SV_Target1;
  float4 wDiffuse : SV_Target2;
};

PSOutput main(VSOutput i) {
  PSOutput o;
  o.wPos = i.wPos;
  o.wNormal = i.wNormal;
  o.wDiffuse = BasicColor.Sample(linearSampler, i.uv);
  return o;
}
