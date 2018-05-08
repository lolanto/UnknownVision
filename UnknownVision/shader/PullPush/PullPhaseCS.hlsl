
Texture2D<unorm float4> LastLevel : register(t0);

RWTexture2D<unorm float4> curLevel : register(u0);

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  // 当前线程的UV
  uint2 tuv = GID.xy * 10 + GTID.xy;
  // 构成当前像素的上一层4个像素
  float4 frags[4];
  frags[0] = LastLevel.Load(uint3(tuv * 2 + uint2(0, 0), 0));
  frags[1] = LastLevel.Load(uint3(tuv * 2 + uint2(1, 0), 0));
  frags[2] = LastLevel.Load(uint3(tuv * 2 + uint2(0, 1), 0));
  frags[3] = LastLevel.Load(uint3(tuv * 2 + uint2(1, 1), 0));
  // 输出的结果
  float4 res = float4(0, 0, 0, 0);
  float weights = 0;
  for (uint i = 0; i < 4; ++i) {
    if (any(frags[i].xyz)) {
      res += frags[i];
      ++weights;
    }
  }
  res = res / weights;
  curLevel[tuv] = res;
}
