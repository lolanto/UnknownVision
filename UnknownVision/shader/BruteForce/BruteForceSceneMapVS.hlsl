#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 uvPos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 worldNor : TEXCOORD1;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.uvPos = float4(i.texcoord * 2.0f - 1.0f, 0.5f, 1.0f);
  o.uvPos.y = -o.uvPos.y;
  o.worldPos = mul(GModelMatrix, float4(i.position, 1.0f));
  o.worldPos.a = i.texcoord.x;
  o.worldNor = mul(GModelMatrix, float4(i.normal, 0.0f));
  o.worldNor.a = i.texcoord.y;

  return o;
}
