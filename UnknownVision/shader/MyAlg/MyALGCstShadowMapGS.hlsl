#include "./MyALGShadowMapHead.hlsli"

struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4 vRef;
  float4 wNor;
  float4x4 refMatrix;
  float4x4 refProjMatrix;
};

StructuredBuffer<RefEleData> RefViewData : register(t0);

cbuffer ExtraData : register(b0) {
  // xy: reflect Point size; z: numOfReflect points
  float4 ExData;
  // xy: 一块shadow map的大小; zw: shadow map整体大小
  float4 ShadowMapSize;
}

struct VSOutput {
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float4 wDiffuse : TEXCOORD2;
};


struct GSOutput {
  float4 sPos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float4 wDiffuse : TEXCOORD2;
};

[maxvertexcount(20)]
[instance(10)]
void main(
  in point VSOutput input[1],
  uint InstanceID : SV_GSInstanceID,
  inout PointStream<GSOutput> output) {
  if (input[0].wPos.w < 0.000001f) return;
  const float2 refPntInv = 1.0f / ExData.xy;
  const float2 refPntInvFlipY = float2(refPntInv.x, -refPntInv.y);
  uint2 iter = uint2(0, InstanceID);
  for(iter.x = 0; iter.x < ExData.x; ++iter.x) {
    float4x4 viewMatrix = RefViewData[iter.x + iter.y * ExData.x].refMatrix;
    float4 vPos = mul(viewMatrix, input[0].wPos);
    if (vPos.z < 0) continue;
    orthogonalMapping(vPos);
    if (vPos.w < 0.000001f) continue;
    vPos.xy *= refPntInv;
    vPos.xy += float2(-1.0f ,1.0f) + refPntInvFlipY + 2 * refPntInvFlipY * iter;

    GSOutput o;
    o.sPos = vPos;
    o.wPos = input[0].wPos;
    o.wNor = input[0].wNor;
    o.wDiffuse = input[0].wDiffuse;
    output.Append(o);
    output.RestartStrip();
  }

}