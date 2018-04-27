// 考虑当前的每个像素应该属于哪个反射点(从哪个反射位置采样)
#include "./MyALGCalAscriptionHead.hlsli"

StructuredBuffer<RefEleData> RefViewMatrixs : register(t0);

Texture2D<float4> SSPos : register(t1);
Texture2D<float4> SSNor : register(t2);

RWTexture2D<unorm float4> AsciptionMap : register(u0);

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

  float3 curWorldPos = SSPos[tuv].xyz;
  float3 curWorldNor = normalize(SSNor[tuv].xyz);
  float3 curWorldRef = reflect(normalize(curWorldPos - GEyePos.xyz), curWorldNor);
  uint minRef = 0;

  const float2 refSize = RefPntData.xy / RefPntData.zw;

  uint2 refPnt = uint2(tuv / refSize);

  minRef = CalculateAscription(refPnt, RefPntData.z,
    curWorldPos, curWorldRef, RefViewMatrixs);

  float result = minRef * 3.6f;
  float x = 1 - abs((result / 60.0f) % 2 - 1);
  float3 re = float3(0, 0, 0);
  if (result < 60) re = float3(1, x, 0);
  else if(result < 120) re = float3(x, 1, 0);
  else if(result < 180) re = float3(0, 1, x);
  else if(result < 240) re = float3(0, x, 1);
  else if(result < 300) re = float3(x, 0, 1);
  else re = float3(1, 0, x);
  AsciptionMap[tuv] = float4(re, 1.0f);
}
