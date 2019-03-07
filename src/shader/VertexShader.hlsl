#include "VS_INPUT.hlsli"

struct v2p {
	float4 position : SV_POSITION;
	float4 t2w0 : TEXCOORD0;
	float4 t2w1 : TEXCOORD1;
	float4 t2w2 : TEXCOORD2;
	float2 texcoord : TEXCOORD3;
};

v2p main( a2v i )
{
	v2p o;
	float4 vPos = mul(GViewMatrix, mul(GModelMatrix, float4(i.position, 1.0)));
	o.position = mul(GProjectMatrix, vPos);
	// 注意! 法线从模型空间到世界空间应该乘以法线的变换矩阵
	float3 viewNor = normalize(mul(GViewMatrix, mul(GModelMatrix, float4(i.normal, 0.0)))).xyz;
	float3 viewTan = normalize(mul(GViewMatrix, mul(GModelMatrix, float4(i.tangent, 0.0)))).xyz;
	float3 viewBinor = normalize(cross(viewNor, viewTan));
	o.t2w0 = float4(viewTan, vPos.x);
	o.t2w1 = float4(viewBinor, vPos.y);
	o.t2w2 = float4(viewNor, vPos.z);
	o.texcoord = i.texcoord;
	return o;
}