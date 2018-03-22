#include "../PS_INPUT.hlsli"

struct GSOutput {
  float4 tPos : SV_POSITION;
  float4 position : TEXCOORD0;
  float2 texcoord : TEXCOORD1;
  uint slide : SV_RenderTargetArrayIndex;
};

struct PSOutput {
  float4 color : SV_Target;
};

Texture2D basicColor : register (t0);
Texture2DArray depthTexArray : register (t1);
SamplerState pointSampler : register (s0);

PSOutput main( GSOutput i ) {
  PSOutput o;
  o.color = basicColor.Sample(pointSampler, i.texcoord);
  switch (i.slide) {
    case 0:
      return o;
      break;
    case 1:
      // to screen space position
      float3 ssp = float3(i.position.xy / i.position.w, i.position.w);
      ssp.xy = (ssp.xy + 1.0f) / 2.0f;
      ssp.y = 1.0f - ssp.y;
      float lastZValue = depthTexArray.Sample(pointSampler, float3(ssp.xy, 0));
      float nf = GCameraParam.x * GCameraParam.y;
      float nSubf = GCameraParam.x - GCameraParam.y;
      lastZValue = nf / ((lastZValue + GCameraParam.y / nSubf) * nSubf);
      if (ssp.z > lastZValue + 0.3f) return o;
      else discard;
      return o;
      break;
  }
  return o;
}

