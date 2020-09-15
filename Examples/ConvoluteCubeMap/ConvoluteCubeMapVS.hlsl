struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput input) {
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.position.xy;
    return output;
}
