Texture2D<unorm float4> LastLevel : register(t0);
Texture2D<unorm float4> RefCurLevel : register(t1);
RWTexture2D<unorm float4> curLevel : register(u0);

[numthreads(10, 10 ,1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;
  float4 res = RefCurLevel.Load(uint3(tuv, 0));
  if (!any(res.xyz)) {
    uint2 base[4];
    base[0] = tuv / 2;
    uint nx = float(tuv.x) / 2.0f > 0.000001f ?
      base[0].x + 1 : base[0].x - 1;
    uint ny = float(tuv.y) / 2.0f > 0.000001f ?
      base[0].y + 1 : base[0].y - 1;
    base[1] = uint2(nx, base[0].y);
    base[2] = uint2(base[0].x, ny);
    base[3] = uint2(nx, ny);
    res = float4(0, 0, 0, 0);
    res += LastLevel.Load(uint3(base[0], 0)) * 9;
    res += LastLevel.Load(uint3(base[1], 0)) * 3;
    res += LastLevel.Load(uint3(base[2], 0)) * 3;
    res += LastLevel.Load(uint3(base[3], 0));
    res /= 16;
  }
  curLevel[tuv] = res;
}