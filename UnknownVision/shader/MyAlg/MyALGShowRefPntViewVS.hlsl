#include "../VS_INPUT.hlsli"

struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4 vRef;
  float4 wNor;
  float4x4 refMatrix;
  float4x4 refProjMatrix;
};

StructuredBuffer<RefEleData> RefViewMatrixs : register(t0);

cbuffer ProjectMatrixData : register(b0) {
  // x: fov, y: aspect, z: near, w: far
  float4 PerProjSettingData;
  float4x4 PerProjMatrix;
  float4x4 PerProjMatrixInv;
  // x: width, y: height, z: near, w: far
  float4 OrtProjSettingData;
  float4x4 OrtProjMatrix;
  float4x4 OrtProjMatrixInv;
}

const static uint RefPntId =77;

struct VSOutput {
  float4 vPos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float4 wPos : TEXCOORD1;
  float4 tNor : TEXCOORD2;
  float4 tPos : TEXCOORD3;
};

VSOutput main(a2v i) {
  VSOutput o;
  o.wPos = mul(GModelMatrix, float4(i.position, 1.0f));
  o.tNor = RefViewMatrixs[RefPntId].wNor;
  o.tPos = RefViewMatrixs[RefPntId].wPos;
  o.vPos = 
      mul(RefViewMatrixs[RefPntId].refMatrix, o.wPos);
  float3 vRef = RefViewMatrixs[RefPntId].vRef.xyz;
  vRef = vRef / -vRef.z;
  // o.vPos.xy += vRef.xy * o.vPos.z;
  o.vPos = mul(PerProjMatrix, o.vPos);
  // o.vPos = mul(OrtProjMatrix, o.vPos);
  // o.vPos = mul(GProjectMatrix, o.vPos);
  o.uv = i.texcoord;
  return o;
}