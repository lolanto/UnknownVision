#include "../VS_INPUT.hlsli"

struct RefEleData{
  float4 wPos;
  float4 wRef;
  float4x4 refMatrix;
};

StructuredBuffer<RefEleData> RefViewMatrixs : register(t0);

cbuffer ProjectMatrixData : register(b0) {
  // 投影矩阵
  float4x4 proMatrix;
}

const static uint RefPntId = 37;

struct VSOutput {
  float4 vPos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.vPos = 
    mul(proMatrix,
      mul(RefViewMatrixs[RefPntId].refMatrix,
      mul(GModelMatrix, float4(i.position, 1.0f))));
  o.uv = i.texcoord;
  return o;
}