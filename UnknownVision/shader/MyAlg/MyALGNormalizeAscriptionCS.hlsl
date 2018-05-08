RWTexture2D<uint> SSIndex : register(u0);

RWStructuredBuffer<int> RWStructuredCounter : register(u1);

[numthreads(10, 10, 1)]
void main(uint3 GTID: SV_GroupThreadID,
  uint3 GID: SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;

  uint e = SSIndex[tuv];
  if (e) {
    uint newSlot = RWStructuredCounter.IncrementCounter();
    SSIndex[tuv] = newSlot;
  }

}