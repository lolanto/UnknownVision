#ifndef PBRFUNCS
#define PBRFUNCS
static const float PI = 3.1430620152f;
static const float3 F0 = float3(0.04f, 0.04f, 0.04f);

// Normal distribution function
float NDF(float NdotNL, float roughness) {
    float a2 = roughness * roughness;
    float NdotH2 = NdotNL * NdotNL;
    float denom = NdotH2 * (a2 - 1) + 1;
    denom *= denom;
    return a2 / (PI * denom);
}

// Gemotry function
float ShlickGGX(float NdotV, float k) {
    return (NdotV / (NdotV * (1 - k) + k));
}

float GFunc(float NdotV, float NdotL, float k) {
    return ShlickGGX(NdotV, k) * ShlickGGX(NdotL, k);
}

// Fresnel equation
float3 Fschlick(float NdotV, float3 F) {
    float p = pow(1 - NdotV, 5);
    return F + (1 - F) * p;
}

// PBR BRDF

float3 PBRSpecular(float NdotV, float NdotL, float NdotNL, 
  float3 orm, float3 Fresnel, float k) {
    float3 nom = NDF(NdotNL, orm.g) * Fresnel * GFunc(NdotV, NdotL, k);
    float denom = max(4 * NdotV * NdotL, 0.0001);
    return nom / denom;
}

// toon mapping
float3 ToonMapping(float3 input) {
    return input / (input + 1);
}

float3 ACESToneMapping(float3 color) {
  const float A = 2.51f;
  const float B = 0.03f;
  const float C = 2.43f;
  const float D = 0.59f;
  const float E = 0.14f;
  // color *= adapted_lum;
  return (color * (A * color + B)) / (color * (C * color + D) + E);
}

// Gamma Decode
float3 GammaDecode(float3 input) {
    return pow(input, float3(2.2, 2.2, 2.2));
}

// Gamma Encode
float3 GammaEncode(float3 input) {
    return pow(input, float3(1 / 2.2, 1 / 2.2, 1 / 2.2));
}

static const float2 invAtan = float2(0.1591f, -0.3183f);
// Equirectangular projection reverse
float2 EqProjReverse(float3 dir) {
  float2 lonlat = float2(atan2(dir.x, dir.z), asin(dir.y));
  lonlat = lonlat * invAtan + 0.5f;
  return lonlat;
}


float3 PBR_BRDF(
  float3 ORM, // occlusion, roughness, metalness
  float3 TexColor, // Texture Diffuse Color(gamma encoded)
  float3 Radiance, // light radiance
  float NdotV, 
  float NdotL, 
  float NdotH,
  float NdotNL,
  bool DirectLight = true
  ) {
  float3 matColor = GammaDecode(TexColor);

  // 根据材质金属性以及颜色推测材质的折射率
  float3 mF0 = lerp(F0, matColor, ORM.z);
  float kDL = DirectLight ? (ORM.g + 1) * (ORM.g + 1) / 8 : (ORM.g * ORM.g) / 2;

  // factor of specular
  float3 Ks = Fschlick(NdotH, mF0);
  // factor of diffuse
  float3 Kd = 1 - Ks;


  float3 Emit = (PBRSpecular(NdotV, NdotL, NdotNL, ORM, Ks, kDL) + 
    Kd * matColor / PI) * Radiance * NdotL;

  return Emit;
}

#endif
