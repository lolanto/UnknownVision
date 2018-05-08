Texture2D<float4> CluPos : register(t0);
Texture2D<float4> CluNor : register(t1);
Texture2D<float4> SSPos : register(t2);
Texture2D<float4> SSNor : register(t3);

RWTexture2D<uint> CluMap : register(u0);

[numthreads(10, 10, 1)]
void main(uint3 GTID: SV_GroupThreadID,
  uint3 GID: SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;

  float3 curPos = SSPos[tuv].xyz;
  float3 curNor = SSNor[tuv].xyz;

  float minDis = float(0xffffffff);
  uint2 minClu = uint2(0, 0);
  for(uint2 iter = uint2(0, 0); iter.y < 10; ++iter.y) {
    for(iter.x = 0; iter.x < 10; ++iter.x) {
      float3 cluPos = CluPos[iter].xyz;
      float3 cluNor = CluNor[iter].xyz;
      float dis = 0.3 * length(cluPos - curPos)
        + 0.7 * acos(dot(cluNor, curNor));
      if (dis < minDis) {
        minDis = dis;
        minClu = iter;
      }
    }
  }
  uint CluId = minClu.x + minClu.y * 10;
  CluMap[tuv] = CluId;
}
