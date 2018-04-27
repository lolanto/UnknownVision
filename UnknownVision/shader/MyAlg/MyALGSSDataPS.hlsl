// 渲染屏幕空间信息，构造G-Buffer
#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
};

struct PSOutput {
  float4 wPos : SV_Target0;
  float4 wNor : SV_Target1;
};

PSOutput main(VSOutput i) {
  PSOutput o;
  o.wPos = i.wPos;
  o.wNor = i.wNor;
  return o;
}
