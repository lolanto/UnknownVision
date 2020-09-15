#ifndef PBR_HEADER_HLSL_INC
#define PBR_HEADER_HLSL_INC

#ifndef PI
#define PI 3.1415926f
#endif // PI

float3 fresnelSchick(const float cosTheta, const float3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistrubitionGGX(const float3 N, const float3 H, const float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (1.0f + roughness);
    float k = (r * r)  / 8.0f;
    float numerator = NdotV;
    float denominator = NdotV * (1.0f - k) + k;
    return numerator / denominator;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

void PBR(
    in const float3 obj2LightDir,
    in const float3 obj2Cam,
    in const float3 normal,
    in const float3 baseColor,
    in const float roughness,
    in const float metallic,
    inout float3 specular,
    inout float3 Kd
) {
    float3 H = normalize(obj2Cam + obj2LightDir);
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, baseColor, metallic);
    float3 F = fresnelSchick(saturate(dot(H, obj2Cam)), F0);
    float NDF = DistrubitionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, obj2Cam, obj2LightDir, roughness);
    float3 numerator = NDF * G * F;
    float denominator = 4.0f * saturate(dot(normal, obj2Cam)) * saturate(dot(normal, obj2LightDir));
    specular = numerator / max(denominator, 0.001f);
    Kd = float3(1.0f, 1.0f, 1.0f) - F;
    Kd *= 1.0f - metallic;
    return;
}

#endif // PBR_HEADER_HLSL_INC
