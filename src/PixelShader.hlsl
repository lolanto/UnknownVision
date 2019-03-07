sampler samp : register(s0);
Texture2D<float4> tex : register(t0);

float4 main() : SV_TARGET
{
	return tex.Sample(samp, float2(1.0f, 0.5f));
}