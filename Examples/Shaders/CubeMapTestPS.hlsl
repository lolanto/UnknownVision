struct PSInput {
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;
};

TextureCube CubeMap : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PSInput input) : SV_TARGET {
    return CubeMap.Sample(LinearSampler, float4(input.texcoord, 0.0f));
}