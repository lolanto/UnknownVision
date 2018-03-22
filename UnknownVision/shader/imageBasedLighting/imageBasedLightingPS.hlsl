#include "../PS_INPUT.hlsli"

struct VSOutput {
  float4 position : SV_POSITION;
  float3 boardPos : TEXCOORD0;
  float3 boardRef : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

struct PSOutput {
  float4 color : SV_Target0;
};

cbuffer IBLBoardData : register (b1) {
  float4 WH;
}

Texture2D basicColor : register (t0);
Texture2D iblTex : register (t1);
SamplerState linearSampler : register (s0);

PSOutput main (VSOutput i) {
  PSOutput o;
  float4 albedo = basicColor.Sample(linearSampler, i.uv);
  float4 diffuseLightColor = float4(0, 0, 0, 0);
  float4 specularLightColor = float4(0, 0, 0, 0);
  o.color = albedo * 0.5;

  if (i.boardPos.z > 0) return o;
  // if (i.boardRef.z < 0) return o;

  // float k = - i.boardPos.z / i.boardRef.z;
  // float2 hitPos = i.boardPos.xy + i.boardRef.xy * k;

  // diffuse
  float3 dir = -i.boardPos.xyz;
  dir.z += 1;
  dir = normalize(dir);
  float k = -i.boardPos.z / dir.z;
  float3 hitPos = i.boardPos + dir * k;

  float len = length(hitPos - i.boardPos);
  len = 1 / (1.0f + 0.35 * len + 0.44 * len * len);

  hitPos.xy = hitPos.xy / WH.xy + 0.5f;
  hitPos.y = 1.0f - hitPos.y;
  if (hitPos.x > 0.0f && hitPos.x < 1.0f
    && hitPos.y > 0.0f && hitPos.y < 1.0f)
  diffuseLightColor = iblTex.Sample(linearSampler, hitPos.xy);

  // specular
  if (i.boardRef.z > 0) {
    k = - i.boardPos.z / i.boardRef.z;
    hitPos = i.boardPos + i.boardRef * k;
    hitPos.xy = hitPos.xy / WH.xy + 0.5f;
    hitPos.y = 1.0f - hitPos.y;
    if (hitPos.x > 0.0f && hitPos.x < 1.0f
      && hitPos.y > 0.0f && hitPos.y < 1.0f)
    specularLightColor = iblTex.Sample(linearSampler, hitPos.xy);
  }
  
  o.color = diffuseLightColor * len + specularLightColor + albedo * 0.5;

  return o;
}
