Texture2D<float4> SSPos : register(t0);
Texture2D<float4> SSNor : register(t1);

RWTexture2D<uint> CluCount : register(u0);

RWTexture2D<int> CluPosX : register(u1);
RWTexture2D<int> CluPosY : register(u2);
RWTexture2D<int> CluPosZ : register(u3);

RWTexture2D<int> CluNorX : register(u4);
RWTexture2D<int> CluNorY : register(u5);
RWTexture2D<int> CluNorZ : register(u6);

RWTexture2D<uint> CluMutex : register(u7);

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {

  uint2 tuv = GID.xy * 10 + GTID.xy;
  float3 curPos = SSPos[tuv].xyz;
  float3 curNor = SSNor[tuv].xyz;

  uint2 centerBias = uint2(0, 0);
  SSPos.GetDimensions(centerBias.x, centerBias.y);
  centerBias /= 10;
  float minDis = float(0xffffffff);
  uint2 minCluster = uint2(0, 0);
  for(uint2 iter = uint2(0, 0); iter.y < 10; ++iter.y) {
    for(iter.x = 0; iter.x < 10; ++iter.x) {
      float3 tempCenterPos = SSPos[iter * centerBias].xyz;
      float3 tempCenterNor = SSNor[iter * centerBias].xyz;
      float dis = 0.3 * length(tempCenterPos - curPos)
        + 0.7 * acos(dot(curNor, tempCenterNor));
      if(minDis > dis) {
        minDis = dis;
        minCluster = iter;
      }
    }
  }

  InterlockedAdd(CluPosX[minCluster], int(curPos.x * 1000.0f));
  InterlockedAdd(CluPosY[minCluster], int(curPos.y * 1000.0f));
  InterlockedAdd(CluPosZ[minCluster], int(curPos.z * 1000.0f));

  InterlockedAdd(CluNorX[minCluster], int(curNor.x * 1000.0f));
  InterlockedAdd(CluNorY[minCluster], int(curNor.y * 1000.0f));
  InterlockedAdd(CluNorZ[minCluster], int(curNor.z * 1000.0f));

  InterlockedAdd(CluCount[minCluster], 1);
}
