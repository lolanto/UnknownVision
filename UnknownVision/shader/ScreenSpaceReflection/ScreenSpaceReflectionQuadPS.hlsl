#include "./ScreenSpaceRayTracing.hlsli"

static const float MaxDistance = 60.0f;
static const float MaxSteps = 30.0f;

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

struct PSOutput {
  float4 color1 : SV_Target0;
};

Texture2D csZBuffer : register (t0);
Texture2D SSBasicColor : register (t1);
Texture2D SSNor : register (t2);
Texture2D SSPos : register (t3);

float distanceSquare(float2 a, float2 b) {
  a -= b;
  return dot(a, a);
}

PSOutput main (VSOutput i) {
  PSOutput output;
  output.color1 = float4(0, 0, 0, 1);

  float3 orgPos = SSPos.Sample(SSRT_PointSampler, i.uv).xyz;
  // float3 orgPos = reconstructCSPos(i.uv, csZBuffer, true);
  float3 orgNor = SSNor.Sample(SSRT_PointSampler, i.uv).xyz;
  float3 orgDir = normalize(reflect(normalize(orgPos), orgNor));
  // output.color1 = float4(orgDir, 1.0f);
  // return output;

  float2 hitPnt = float2(0, 0);
  float3 csHitPnt = float3(0, 0, 0);
  float resType = -1;

  if (traceScreenSpaceRay(
    orgPos, orgDir, csZBuffer, true, 1.0f, 0.1f, 0.05f, MaxSteps, MaxDistance,
    hitPnt, resType)) {

    output.color1 = SSBasicColor.Sample(SSRT_PointSampler, hitPnt);
  }
  if (resType == 0.0f) output.color1 = float4(0.2f, 0.2f, 0.2f, 1.0f);
  if (resType == 1.0f) output.color1 = float4(0.0f, 0.0f, 0.0f, 1.0f);
  return output;
}
