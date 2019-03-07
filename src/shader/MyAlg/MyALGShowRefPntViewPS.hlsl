struct VSInput {
  float4 vPos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float4 wPos : TEXCOORD1;
  float4 tNor : TEXCOORD2;
  float4 tPos : TEXCOORD3;
};

Texture2D BasicColor : register(t0);
SamplerState linearSampler : register(s0);

float4 main(VSInput i) : SV_Target {
  if (dot(i.wPos.xyz - i.tPos.xyz, i.tNor.xyz) < 0) discard;
  return BasicColor.Sample(linearSampler, i.uv);
}
