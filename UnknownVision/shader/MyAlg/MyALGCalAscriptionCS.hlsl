// 考虑当前的每个像素应该属于哪个反射点(从哪个反射位置采样)
#include "./MyALGCalAscriptionHead.hlsli"

StructuredBuffer<RefEleData> RefViewMatrixs : register(t0);

Texture2D<float4> SSPos : register(t1);
Texture2D<float4> SSNor : register(t2);
Texture2D<uint> SSID : register(t3);
Texture2D<uint> SSIndex : register(t4);

RWTexture2D<uint> AscriptionData : register(u0);

cbuffer CameraData : register(b0) {
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

cbuffer RefPntData : register(b1) {
  // xy: Screen Size; zw: number of Ref points per Row and Column
  float4 RefPntData;
}

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;

  // float3 curWorldPos = SSPos[tuv].xyz;
  // float3 curWorldNor = normalize(SSNor[tuv].xyz);
  // float3 curWorldRef = reflect(normalize(curWorldPos - GEyePos.xyz), curWorldNor);
  // uint minRef = 0;

  // const float2 refSize = RefPntData.xy / RefPntData.zw;

  // uint2 refPnt = uint2(tuv / refSize);

  // minRef = CalculateAscription(refPnt, RefPntData.z,
  //   curWorldPos, curWorldRef, curWorldNor, RefViewMatrixs);

  int2 ScreenSize = GCameraParam.zw;
  uint faceID = SSID[tuv];
  // if (faceID == uint(0xffffff)) {
  if (faceID > 10000000) {
    AscriptionData[tuv] = uint(0xffffff);
    return;
  }
  // 当前所在的块
  uint2 tile = tuv / (ScreenSize / uint2(5, 5));
  uint2 slot = uint2((tile.x + tile.y * 5) * 36 + faceID, 0);

  uint minRef = SSIndex[slot];

  AscriptionData[tuv] = minRef;
}
