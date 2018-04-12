
struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

Texture2D TracingResult : register (t0);
SamplerState pointSampler : register (s0);

float4 main (VSOutput i) : SV_Target {
  return TracingResult.Sample(pointSampler, i.uv);
}
