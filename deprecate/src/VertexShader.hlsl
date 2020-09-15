struct mats {
	float4x4 model;
	float4x4 view;
	float4x4 projection;
};

ConstantBuffer<mats> g_matrix : register(b0);
ConstantBuffer<mats> b_matrix : register(b2);

float4 main( float3 pos : POSITION ) : SV_POSITION
{
	// 中文注释
	float4x4 t = mul(g_matrix.model, b_matrix.view);
	return mul(t, float4(pos, 1.0f));

}