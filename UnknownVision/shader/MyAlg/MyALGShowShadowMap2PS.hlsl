struct GSInput {
  float4 pos : SV_POSITION;
  uint viewPortIndex : SV_ViewportArrayIndex;
  float2 uv : TEXCOORD0;
};

struct PSOutput {
  float4 diffuse : SV_Target0;
};

Texture2D BasicColor : register(t0);
SamplerState linearSampler : register(s0);

PSOutput main(GSInput i) {
  PSOutput o;
  o.diffuse = BasicColor.Sample(linearSampler, i.uv);
  return o;
}