struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> image1 : register(t0);
Texture2D<float4> image2 : register(t1);
SamplerState linearSampler : register(s0);

static const float WIDTH = 1280.0f;
static const float HEIGHT = 800.0f;

float4 main(VSOutput input) : SV_Target {
    float2 pos0 = input.texcoord;

    float2 pos1 = input.texcoord;

    return (image1.Sample(linearSampler, pos0, int2(0, 0)) 
        + image1.Sample(linearSampler, pos0, int2(1, 0))
        + image2.Sample(linearSampler, pos1, int2(0, 0))
        + image2.Sample(linearSampler, pos1, int2(0, -1))) / 4.0;
}