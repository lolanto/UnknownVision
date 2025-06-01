struct ViewportDataStructure
{
    float4 position;
    float4x4 viewMat;
    float4x4 projMat;
};

/* #ViewportData0# */

struct VertexData
{
    float3 Position;
    float2 Texcoord;
    float3 Normal;
};

/* #VERTEX_DECLARATIONS# */

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

/* #ConstantBuffer0# */

#ifdef VERTEX_SHADER

VSOutput mainVS(VertexInput input) {
    VertexData resolvedVertexData = GetVertexData(input);
    VSOutput output;
    output.position = 
        mul(ViewportData.projMat,
            mul(ViewportData.viewMat, float4(resolvedVertexData.Position, 1.0f)));
    output.texcoord = resolvedVertexData.Texcoord;
    return output;
}

#endif // VERTEX_SHADER

/* #TextureBuffer0# */
SamplerState linearSampler : register(s0);

#ifdef PIXEL_SHADER

float4 mainPS(VSOutput input) : SV_Target {
    return image.Sample(linearSampler, input.texcoord);
}

#endif // PIXEL_SHADER
