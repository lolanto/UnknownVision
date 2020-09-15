#include "../PS_INPUT.hlsli"
#include "../Utility/Utility_PBRFuncs.hlsli"

Texture2D WorldPosition : register(t0);
Texture2D WorldNormal : register(t1);
Texture2D<float2> UV : register(t2);
Texture2D Diffuse : register(t3);
Texture2D ORM : register(t4);
Texture2D ShadowMap : register(t5);
Texture2D ReflectionRadiance : register(t6);
Texture2D ReflectionPos : register(t7);

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

cbuffer ShadowMatrixs : register(b1) {
  float4 lightPosAndInside;
  float4 lightColor;
  float4 lightOrientAndOutside;
  float4x4 viewMatrix;
  float4x4 projMatrix;
}

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

const static float3 AmbientRadiance = float3(0.001f, 0.001f, 0.001f);

float4 main(VSOutput i) : SV_Target {

  float4 worldPos = WorldPosition.Sample(pointSampler, i.uv);
  if (worldPos.w < 10e-6) discard;

  float3 worldNor = WorldNormal.Sample(pointSampler, i.uv).xyz;
  float3 viewDir = normalize(GEyePos.xyz - worldPos.xyz);
  float2 texUV = UV.Sample(pointSampler, i.uv).xy;
  float3 curORM = ORM.Sample(pointSampler, texUV).xyz;
  float3 lightRadiance = lightColor.xyz;
  float3 diffuseColor = Diffuse.Sample(linearSampler, texUV).rgb;

  float4 clipPos = mul(viewMatrix, worldPos);
  if (clipPos.z < 0) lightRadiance *= 0;

  clipPos = mul(projMatrix, clipPos);

  if (abs(clipPos.x) > clipPos.w ||
    abs(clipPos.y) > clipPos.w ||
    abs(clipPos.z) > clipPos.w) lightRadiance *= 0;

  clipPos /= clipPos.w;
  clipPos.y = -clipPos.y;
  clipPos.xy = (clipPos.xy + 1.0f) / 2.0f;
  float2 clipPosTexcel;
  ShadowMap.GetDimensions(clipPosTexcel.x, clipPosTexcel.y);
  clipPosTexcel *= clipPos.xy;
  clipPos.z -= 0.01f;
  float visibility = 1.0f;
  for(float2 iter = float2(-1.0f, -1.0f); iter.y < 1.9f; ++iter.y) {
    for(iter.x = -1.0f; iter.x < 1.9f; ++iter.x) {
      float depth = ShadowMap[iter + clipPosTexcel].r;
      if (depth <= clipPos.z) visibility -= 0.1f;
    }
  }
  lightRadiance *= visibility;
  // float dpeth = ShadowMap.Sample(pointSampler, clipPos.xy).r;
  // if (dpeth + 0.006 <= clipPos.z) lightRadiance *= 0;

  
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

  float3 EmitRadiance = 0.1f * PBR_BRDF(
    curORM,
    diffuseColor,
    lightRadiance,
    NdotV, NdotL, NdotH,NdotNL
    );

  // from reflection
  float4 refPos = ReflectionPos.Sample(pointSampler, i.uv);
  lightDir = refPos.xyz - worldPos.xyz;
  dist = length(lightDir);
  lightDir /= dist;
  float attenuation = 1.0f / (1.0f + 0.09 * dist + 0.032 * dist * dist);
  lightRadiance = ReflectionRadiance.SampleLevel(linearSampler, i.uv, 0).rgb * attenuation;
  
  HalfViewAndLight = normalize(viewDir + lightDir);
  HalfNormalAndLight = normalize(worldNor + lightDir);
  NdotL = max(dot(worldNor, lightDir), 0.0f);
  NdotH = max(dot(worldNor, HalfViewAndLight), 0.0f);
  NdotNL = max(dot(worldNor, HalfNormalAndLight), 0.0f);
  
  EmitRadiance += PBR_BRDF(
    curORM,
    diffuseColor,
    lightRadiance,
    NdotV, NdotL, NdotH,NdotNL
    );

  EmitRadiance *= curORM.x;
  EmitRadiance = ACESToneMapping(EmitRadiance);
  EmitRadiance = GammaEncode(EmitRadiance);

  return float4(EmitRadiance, 1.0f);
}