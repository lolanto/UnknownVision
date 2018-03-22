#include "../VS_INPUT.hlsli"

struct v2p {
	float4 position : SV_POSITION;
	float4 eye : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
};

v2p main( a2v i )
{
	v2p o;
	o.position = float4(i.position, 1.0f);
	o.eye = GEyePos;
	o.texcoord = i.texcoord;
	return o;
}