
struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

Texture2D TracingResult : register (t0);
SamplerState linearSampler : register (s0);

float4 main (VSOutput i) : SV_Target {
  float2 TexSize;
  TracingResult.GetDimensions(TexSize.x, TexSize.y);
  TexSize = 1.0f / TexSize;
  float4 res = float4(0, 0, 0, 0);
  for(int row = -1; row < 2; ++row) {
    for(int col = -1; col < 2; ++col) {
      res += TracingResult.Sample(linearSampler, i.uv + float2(col * TexSize.x, row * TexSize.y)) / 9;
    }
  }
  return res;
}
