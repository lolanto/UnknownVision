struct VSInput {
  float4 vPos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

Texture2D BasicColor : register(t0);
SamplerState linearSampler : register(s0);

float4 main(VSInput i) : SV_Target {
  return BasicColor.Sample(linearSampler, i.uv);
}
