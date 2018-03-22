#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 position : SV_POSITION;
  float2 texcoord : TEXCOORD0;
};

cbuffer CubeMapData : register (b3) {
  matrix basicViewMat;
}

VSOutput main( a2v i )
{
	VSOutput o;
  o.position = 
  mul(basicViewMat,
    mul(GModelMatrix, float4(i.position, 1.0f)));
  // o.texcoord = i.texcoord;
    // mul(basicViewMat,
    //   mul(GModelMatrix, float4(i.position, 1.0f)));
  o.texcoord = i.texcoord;
  return o;
}