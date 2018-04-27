#include "./MyALGShadowMapHead.hlsli"
Texture2D<uint> AscriptionData : register(t0);
Texture2D<unorm float4> ShadowMapDiffuse : register(t1);

Texture2D<float4> SSPos : register(t2);
Texture2D<float4> SSNor : register(t3);

Texture2D<float4> ScePntPos : register(t4);
Texture2D<float4> ScePntNor : register(t5);

struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4x4 refMatrix;
};
StructuredBuffer<RefEleData> RefViewData : register(t6);

Texture2D<float4> ShadowMapPos : register(t7);
Texture2D<float4> ShadowMapNor : register(t8);

RWTexture2D<float4> RefResPos : register(u0);

cbuffer ShadowMapData : register(b0) {
  // xy: reflect Point size; z: numOfReflect points
  float4 ShadowMapData;
  // xy: 一块shadow map的大小; zw: shadow map整体大小
  float4 ShadowMapSize;
}

cbuffer CameraData : register(b1) {
  matrix GViewMatrix;
  matrix GViewMatrixInv;
  matrix GProjectMatrix;
  matrix GProjectMatrixInv;
  float4 GEyePos;
  // x: n, y: f, z: width, w: height
  float4 GCameraParam;
  /* 
  xy: nearPlaneSize.xy
  zw: farPlaneSize.xy
  */
  float4 GCameraParam2;
}

cbuffer ProjectMatrixData : register(b2) {
  float4x4 ProjMatrix;
}

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {

  uint2 tuv = GID.xy * 10 + GTID.xy;
  // 无效点
  if (SSPos[tuv].w < 0.000001f) return;

  float3 pntPos = SSPos[tuv].xyz;
  float3 pntNor = SSNor[tuv].xyz;
  float3 pntRef = reflect(normalize(pntPos - GEyePos.xyz), pntNor);

  const float2 SmDataInv = float2(1.0f / ShadowMapData.xy);

  // 当前点所属的反射样本点ID
  uint ascripPnt = AscriptionData[tuv].x;
  // 求出当前反射样本点的下标
  float2 RefPntIdx2 = float2(ascripPnt % ShadowMapData.x,
    floor(ascripPnt / ShadowMapData.x));
  // // 求出当前反射样本的shadow map的中心坐标
  // uint2 texPos = RefPntIdx2 * ShadowMapSize.xy;
  // const uint2 halfSubShadowMapSize = ShadowMapSize.xy / 2;
  // texPos += halfSubShadowMapSize;

  // !判断当前获得的场景点的坐标是否有效

  // float3 tarScePntPos = ScePntPos[scePntTexPos].xyz;
  // float3 tarScePntNor = ScePntNor[scePntTexPos].xyz;

  // 反射样本点的反射方向
  // const float bias = 3.0f;
  // float3 RefPntRefDir = normalize(RefViewData[ascripPnt].wRef.xyz);
  // float4 samplePnt = float4(RefViewData[ascripPnt].wPos.xyz
  //   + pntRef * bias / dot(RefPntRefDir, pntRef), 1.0f);
  // samplePnt = mul(RefViewData[ascripPnt].refMatrix, samplePnt);
  // orthogonalMapping(samplePnt);
  // fromNDCtoViewSpace(samplePnt);

  // samplePnt.xy /= ShadowMapData.xy;
  // samplePnt.xy += RefPntIdx2 * SmDataInv;
  // uint2 texPos = samplePnt.xy * ShadowMapSize.zw;

  const float bias = 3.0f;
  float4 RefPntPos = float4(pntPos + pntRef * bias, 1.0f);
  RefPntPos = mul(ProjMatrix, mul(RefViewData[ascripPnt].refMatrix, RefPntPos));
  RefPntPos /= RefPntPos.w;
  RefPntPos.xy = clamp(RefPntPos.xy, float2(-1, -1), float2(1, 1));
  RefPntPos.y = -RefPntPos.y;

  uint2 texPos = (RefPntPos.xy + 1) * 0.5f * ShadowMapSize.xy
    + RefPntIdx2 * ShadowMapSize.xy;

  // 阴影图上缺失反射信息无法采样
  if (ShadowMapPos[texPos].w < 0.00001f) {
    RefResPos[tuv] = float4(0.5, 0.5, 0.5, 0.5);
    return;
  };

  float3 refPos = ShadowMapPos[texPos].xyz;
  float3 refNor = ShadowMapNor[texPos].xyz;

  float t = dot(refPos - pntPos, refNor) / dot(pntRef, refNor);
  RefResPos[tuv] = float4(pntPos + t * pntRef, 1.0f);

  // RefResPos[tuv] = ShadowMapDiffuse[texPos];
}