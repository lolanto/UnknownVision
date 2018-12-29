float4 main( float3 pos : POSITION ) : SV_POSITION
{
	// 中文注释
	return float4(pos, 1.0f);
}