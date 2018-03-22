#include "../VS_INPUT.hlsli"

struct VSOutput {
  float4 position : SV_POSITION;
  float3 boardPos : TEXCOORD0;
  float3 boardRef : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

cbuffer IBLBoardData : register(b3) {
  matrix BoardMatrix;
}

VSOutput main (a2v i) {
  VSOutput o;

  float3 worldPos = 
  mul(GModelMatrix, float4(i.position, 1.0f)).xyz;

  float3 worldNor = 
  mul(GModelMatrix, float4(i.normal, 0.0f)).xyz;
  worldNor = normalize(worldNor);
  float3 refDir = reflect(worldPos - GEyePos.xyz, worldNor);
  refDir = normalize(refDir);

  o.position = 
  mul(GProjectMatrix,
    mul(GViewMatrix, float4(worldPos, 1.0f)));
  o.uv = i.texcoord;

  float3 boardPos = 
  mul(BoardMatrix, float4(worldPos, 1.0f)).xyz;
  float3 boardRef =
  mul(BoardMatrix, float4(refDir, 0.0f)).xyz;
  boardRef = normalize(boardRef);

  o.boardPos = boardPos;
  o.boardRef = boardRef;

  return o;
}
