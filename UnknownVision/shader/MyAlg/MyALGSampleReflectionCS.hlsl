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
  float4 vRef;
  float4 wNor;
  float4x4 refMatrix;
  float4x4 refProjMatrix;
};
StructuredBuffer<RefEleData> RefViewData : register(t6);

Texture2D<float4> ShadowMapPos : register(t7);
Texture2D<float4> ShadowMapNor : register(t8);
Texture2D ShadowMapDepth : register(t9);
Texture2D<float4> ShadowMapTan : register(t10);
Texture2D<float4> ShadowMapBin : register(t11);
Texture2D<float4> ShadowMapUV : register(t12);


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
  // x: fov, y: aspect, z: near, w: far
  float4 PerProjSettingData;
  float4x4 PerProjMatrix;
  float4x4 PerProjMatrixInv;
  // x: width, y: height, z: near, w: far
  float4 OrtProjSettingData;
  float4x4 OrtProjMatrix;
  float4x4 OrtProjMatrixInv;
}

// a: (nf) / (n - f)
// b: (f) / (f - n)
float ToLinearDepth(float a, float b, float depth);

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
  if (ascripPnt == 0xffffffffu) return;
  // if (ascripPnt > 10000000) return;

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

  // const float bias = 0.01f;
  // float4 RefPntPos = float4(pntPos + pntRef * bias, 1.0f);

  // RefPntPos = mul(RefViewData[ascripPnt].refMatrix, RefPntPos);
  // float3 vRef = RefViewData[ascripPnt].vRef.xyz;
  // vRef = vRef / -vRef.z;
  // RefPntPos.xy += vRef.xy * RefPntPos.z;
  // RefPntPos = mul(RefViewData[ascripPnt].refProjMatrix, RefPntPos);

  // RefPntPos /= RefPntPos.w;
  // RefPntPos.xy = clamp(RefPntPos.xy, float2(-1, -1), float2(1, 1));
  // RefPntPos.y = -RefPntPos.y;


  const float bias = 1.0f;
  float4 CamSpaceOriPntPos = float4(pntPos, 1.0f);
  float4 CamSpaceRefPntPos = float4(pntPos + pntRef * bias, 1.0f);

  CamSpaceOriPntPos = mul(RefViewData[ascripPnt].refMatrix, CamSpaceOriPntPos);
  CamSpaceRefPntPos = mul(RefViewData[ascripPnt].refMatrix, CamSpaceRefPntPos);

  float3 vRef = RefViewData[ascripPnt].vRef.xyz;
  vRef /= -vRef.z;
  // CamSpaceRefPntPos.xy += vRef.xy * CamSpaceRefPntPos.z;
  // CamSpaceOriPntPos.z应该是0
  // CamSpaceOriPntPos.xy += vRef.xy * CamSpaceOriPntPos.z;


  float3 CamSpaceDirection = normalize(CamSpaceRefPntPos.xyz - CamSpaceOriPntPos.xyz);
  // 现在OriPntPos以及RefPntPos均在裁剪空间中
  float4 ClipSpaceOriPntPos = mul(PerProjMatrix, CamSpaceOriPntPos);
  // float4 ClipSpaceRefPntPos = mul(PerProjMatrix, CamSpaceRefPntPos);

  // float4 ClipSpaceRefPntPos = mul(OrtProjMatrix, CamSpaceRefPntPos);
  // float4 ClipSpaceOriPntPos = mul(OrtProjMatrix, CamSpaceOriPntPos);

  // float4 ClipSpaceOriPntPos = mul(GProjectMatrix, CamSpaceOriPntPos);

  float3 NDCOriPnt = ClipSpaceOriPntPos.xyz / ClipSpaceOriPntPos.w;
  NDCOriPnt.xy = clamp(NDCOriPnt.xy, float2(-1, -1), float2(1, 1));
  // float3 NDCRefPnt = ClipSpaceRefPntPos.xyz / ClipSpaceRefPntPos.w;
  // NDCRefPnt.xy = clamp(NDCRefPnt.xy, float2(-1, -1), float2(1, 1));

  // float2 NDCOffset = NDCRefPnt.xy - NDCOriPnt.xy;
  // NDCOffset = float2(sign(NDCOffset.x), sign(NDCOffset.y));
  // float2 NDCEdgeT = (NDCOffset - NDCOriPnt.xy) / (NDCRefPnt.xy - NDCOriPnt.xy);
  // float minEdgeT = min(NDCEdgeT.x, NDCEdgeT.y);

  // float3 NDCEdge = NDCOriPnt + (NDCRefPnt - NDCOriPnt) * minEdgeT;

  // float EdgeDepth = PerProjMatrix[2][3] + PerProjMatrix[2][2];
  // float3 CamSpaceEdge = mul(PerProjMatrixInv, float4(NDCEdge.xy, EdgeDepth, 1.0f)).xyz;
  // float3 CamSpaceEdgeDirection = normalize(CamSpaceEdge);
  // float3 n = cross(CamSpaceEdgeDirection, cross(CamSpaceDirection, CamSpaceEdgeDirection));
  // n = normalize(n);
  // float dirN = dot(CamSpaceDirection, n);
  // dirN += dirN == 0 ? 0.0001f : 0.0f;
  // float len = dot(CamSpaceEdge - CamSpaceOriPntPos.xyz, n) / dirN;
  // float3 CamSpaceEnd = len * CamSpaceDirection + CamSpaceOriPntPos.xyz;

  // if (CamSpaceEnd.z > PerProjSettingData.w
  //   || len < 0) {
  //   CamSpaceEnd = CamSpaceOriPntPos.xyz + CamSpaceDirection * (PerProjSettingData.w - CamSpaceOriPntPos.z) / CamSpaceDirection.z;
  // }

  // float4 ClipSpaceEdge = mul(PerProjMatrix, float4(CamSpaceEnd, 1.0f));
  // NDCEdge = ClipSpaceEdge.xyz / ClipSpaceEdge.w;
  // NDCEdge.y = -NDCEdge.y;
  NDCOriPnt.y = -NDCOriPnt.y;

  // float deltaZ = NDCEdge.z - NDCOriPnt.z;

  // float2 SSEdge = (NDCEdge.xy + 1) * 0.5f * (ShadowMapSize.xy - 1.0f)
  //   + RefPntIdx2 * ShadowMapSize.xy;
  // float2 SSOri = (NDCOriPnt.xy + 1) * 0.5f * (ShadowMapSize.xy - 1.0f)
  //   + RefPntIdx2 * ShadowMapSize.xy;

  // float2 SSDelta = SSEdge - SSOri;

  // float2 SSCur = SSOri;
  // float SSDepth = NDCOriPnt.z;
  // float SSEnd = SSEdge.x;

  // bool isFlip = false;
  // if (abs(SSDelta.y) > abs(SSDelta.x)) {
  //   isFlip = true;
  //   SSDelta.xy = SSDelta.yx;
  //   SSCur.xy = SSOri.yx;
  //   SSEnd = SSEdge.y;
  // }

  // deltaZ /= abs(SSDelta.x);
  // SSDelta.y /= abs(SSDelta.x);
  // SSDelta.x = sign(SSDelta.x);
  // SSEnd *= sign(SSDelta.x);

  // float curStep = 0;
  // for(;
  //   SSEnd > SSCur.x * SSDelta.x &&
  //   curStep < 24.0f;
  //   ++curStep) {

  //   float depth = ShadowMapDepth[isFlip ? SSCur.yx : SSCur.xy].r;
  //   if (SSDepth > depth) {
  //     if (SSDepth - depth > 0.05) {
  //       RefResPos[tuv] = float4(0.2, 0.2, 0.2, 1);
  //       return;
  //     }
  //     break;
  //   }

  //   SSCur += SSDelta;
  //   SSDepth += deltaZ;
  // }

  // uint2 texPos = isFlip ? SSCur.yx : SSCur.xy;

  // NDCRefPnt.y = -NDCRefPnt.y;
  // uint2 texPos = (NDCRefPnt.xy + 1) * 0.5f * (ShadowMapSize.xy - 1)
  //   + RefPntIdx2 * ShadowMapSize.xy;

  uint2 texPos = (NDCOriPnt.xy + 1) * 0.5f * (ShadowMapSize.xy - 1)
    + RefPntIdx2 * ShadowMapSize.xy;

  // uint2 texPos = (RefPntPos.xy + 1) * 0.5f * ShadowMapSize.xy
  //   + RefPntIdx2 * ShadowMapSize.xy;

  // 阴影图上缺失反射信息无法采样
  if (ShadowMapPos[texPos].w < 0.00001f) {
    RefResPos[tuv] = float4(0.5, 0.5, 0.5, 0.5);
    return;
  };

  float3 refPos = ShadowMapPos[texPos].xyz;
  float3 refNor = ShadowMapNor[texPos].xyz;
  float4 refUV = ShadowMapUV[texPos];

  float t = dot(refPos - pntPos, refNor) / dot(pntRef, refNor);
  float3 tPos = pntPos + t * pntRef;
  float3 dPos = tPos - refPos;
  float2 xUV = float2(dot(dPos, ShadowMapTan[texPos].xyz),
    dot(dPos, ShadowMapBin[texPos].xyz));
  xUV = xUV * refUV.zw + refUV.xy;
  // RefResPos[tuv] = float4(pntPos + t * pntRef, 1.0f);
  RefResPos[tuv] = float4(xUV, 0.0f ,1.0f);

  // RefResPos[tuv] = ShadowMapDiffuse[texPos];
}

float ToLinearDepth(float a, float b, float depth) {
  return a / (depth - b);
}
