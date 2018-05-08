#include "../Utility/Utility_PBRFuncs.hlsli"
#include "../PS_INPUT.hlsli"

Texture2D WorldPosition : register(t0);
Texture2D WorldNormal : register(t1);
Texture2D<float2> UV : register(t2);
Texture2D Diffuse : register(t3);
Texture2D ORM : register(t4);
Texture2D ShadowMap : register(t5);
Texture2D WorldRef : register(t6);

cbuffer ShadowMatrixs : register(b1) {
  float4 lightPosAndInside;
  float4 lightColor;
  float4 lightOrientAndOutside;
  float4x4 viewMatrix;
  float4x4 projMatrix;
}

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

struct VSInput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 main(VSInput i) : SV_Target {
  float4 worldPos = WorldPosition.Sample(pointSampler, i.uv);
  if (worldPos.w < 10e-6) discard;

  float3 worldNor = WorldNormal.Sample(pointSampler, i.uv).xyz;
  float3 viewDir = -normalize(WorldRef.Sample(pointSampler, i.uv).xyz);
  float2 texUV = UV.Sample(pointSampler, i.uv).xy;
  float3 curORM = ORM.Sample(pointSampler, texUV).xyz;
  float3 lightRadiance = lightColor.xyz;

  float4 clipPos = mul(viewMatrix, worldPos);
  if (clipPos.z < 0) lightRadiance *= 0;

  clipPos = mul(projMatrix, clipPos);

  if (abs(clipPos.x) > clipPos.w ||
    abs(clipPos.y) > clipPos.w ||
    abs(clipPos.z) > clipPos.w) lightRadiance *= 0;

  clipPos /= clipPos.w;
  clipPos.y = -clipPos.y;
  clipPos.xy = (clipPos.xy + 1.0f) / 2.0f;
  float dpeth = ShadowMap.Sample(pointSampler, clipPos.xy).r;
  if (dpeth + 0.006 <= clipPos.z) lightRadiance *= 0;
  
  // light dir
  float3 lightDir = worldPos.xyz - lightPosAndInside.xyz;
  float dist = length(lightDir);
  lightDir /= dist;
  float innerAngle = cos(lightPosAndInside.w / 2.0f);
  float cutOff = cos(lightOrientAndOutside.w / 2.0f);
  float angle = dot(lightDir, normalize(lightOrientAndOutside.xyz));
  if (angle < cutOff) lightRadiance *= 0;
  else {
    float attenuation = 1.0f / (1.0f + 0.09 * dist + 0.032 * dist * dist);
    float intensity = 
      clamp((angle - cutOff) / (innerAngle - cutOff), 0.0f, 1.0f);
  
    lightRadiance *= attenuation * intensity;
  }

  float3 HalfViewAndLight = normalize(viewDir - lightDir);
  float3 HalfNormalAndLight = normalize(worldNor - lightDir);
  float NdotV = max(dot(worldNor, viewDir), 0.0f);
  float NdotL = max(dot(worldNor, -lightDir), 0.0f);
  float NdotH = max(dot(worldNor, HalfViewAndLight), 0.0f);
  float NdotNL = max(dot(worldNor, HalfNormalAndLight), 0.0f);

  float3 EmitRadiance = PBR_BRDF(
    curORM,
    Diffuse.Sample(linearSampler, texUV).xyz,
    lightRadiance,
    NdotV, NdotL, NdotH,NdotNL
    );
  EmitRadiance *= curORM.x;
  return float4(EmitRadiance, 1.0f);  
}