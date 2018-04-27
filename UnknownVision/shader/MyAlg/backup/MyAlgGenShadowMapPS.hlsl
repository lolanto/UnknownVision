#include "../PS_INPUT.hlsli"

struct GSOutput{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(GSOutput i) : SV_Target {
  return float4(i.uv, 0.0f ,0.0f);
}
