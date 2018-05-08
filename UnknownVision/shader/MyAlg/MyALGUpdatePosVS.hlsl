// 更新场景中所有点的位置，法线，切线以及副切线资料
#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNormal : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.wPos = mul(GModelMatrix, float4(i.position, 1.0f));
  o.wNormal = normalize(mul(GModelMatrix, float4(i.normal, 0.0f)));

  o.pos = float4(0, 0, 0.5, 1);
  o.pos.xy = (i.texcoord * 2.0f - 1.0f);
  o.pos.y = -o.pos.y;
  o.uv = i.texcoord;
  return o;
}
