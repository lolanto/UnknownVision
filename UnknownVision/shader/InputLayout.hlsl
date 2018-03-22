#include "VS_INPUT.hlsli"

float4 main( a2v i ) : SV_POSITION
{
	return float4(i.position, 1.0);
}