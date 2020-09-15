struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

TextureCube EnvMap : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer ControlData : register(b0) {
    uint Axis;
}

float3 GammaDecode(float3 input) {
    return pow(input, 2.2f);
}

float3 GammaEncode(float3 input) {
    return pow(input, 0.454f);
}
static const float PI = 3.1415926f;
static const float SampleCountTheta = 16.0f;
static const float SampleCountPhi = 48.0f;
static const float ThetaRate = (PI / 2.0f) / SampleCountTheta;
static const float PhiRate = (2.0f * PI) / SampleCountPhi;

float4 main(PSInput input) : SV_TARGET {
    float3 M;
    float2 uv = input.uv;
    switch(Axis) {
    case 0: // +x
        M = float3(1.0f, uv.y, -uv.x);
        break;
    case 1: // -x
        M = float3(-1.0f, uv.y, uv.x);
        break;
    case 2: // +y
        M = float3(uv.x, 1.0f, -uv.y);
        break;
    case 3: // -y
        M = float3(uv.x, -1.0f, uv.y);
        break;
    case 4: // +z
        M = float3(uv.x, uv.y, 1.0f);
        break;
    case 5: // -z
        M = float3(-uv.x, uv.y, -1.0f);
        break;
    }
    M = normalize(M);
    float3 tangent, bitangent;
    if (any(M - float3(0.0f, 0.0f, 1.0f))) {
        tangent = cross(M, float3(0.0f, 0.0f, 1.0f));
        bitangent = cross(tangent, M);
    } else {
        tangent = cross(M, float3(0.0f, -1.0f, 0.0f));
        bitangent = cross(tangent, M);
    }
    float3 totalRadiance = float3(0.0f, 0.0f, 0.0f);
    const float C = (PI) / (SampleCountPhi * SampleCountTheta);
    for(float phi = 0.0f; phi < 2.0f * PI; phi += PhiRate) {
        for(float theta = 0.0f; theta < PI / 2.0f; theta += ThetaRate) {
            float3 tangentSpaceN = float3(cos(phi) * sin(theta), cos(theta), sin(phi) * sin(theta));
            float3 worldSpaceN = tangentSpaceN.x * tangent + tangentSpaceN.y * M + tangentSpaceN.z * bitangent;
            totalRadiance += GammaDecode(EnvMap.Sample(LinearSampler, worldSpaceN)) * cos(theta) * sin(theta);
        }
    }
    totalRadiance = totalRadiance * C;
    return float4(totalRadiance, 1.0f);
}
