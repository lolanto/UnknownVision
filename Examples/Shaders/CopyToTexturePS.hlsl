struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> image : register(t0);
SamplerState linearSampler : register(s0);

float4 main(VSOutput input) : SV_Target {
    return image.Sample(linearSampler, input.texcoord);
}