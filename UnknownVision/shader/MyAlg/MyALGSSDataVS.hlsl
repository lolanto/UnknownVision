// 渲染屏幕空间信息，构造G-Buffer
#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.wPos = mul(GModelMatrix, float4(i.position, 1.0f));
  o.wNor = mul(GModelMatrix, float4(i.normal, 0.0f));
  o.pos = mul(GProjectMatrix,
    mul(GViewMatrix, o.wPos));
  return o;
}
