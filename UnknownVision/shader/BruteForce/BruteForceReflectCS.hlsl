Texture2D SSWorldPos : register (t0);
Texture2D SSWorldRef : register (t1);
Texture2D SMWorldPos : register (t2);

Texture2D SMAlbedo : register (t3);
RWTexture2D<unorm float4> RSAlbedo : register (u0);

struct CSInput {
  uint3 GTID : SV_GroupThreadID;
  uint3 GID : SV_GroupID;
};

[numthreads(32, 32, 1)]
void main(CSInput i) {
  uint2 uv = (32 * i.GID + i.GTID).xy;
  float3 oriPos = SSWorldPos.Load(uint3(uv, 0)).xyz;
  float3 oriDir = SSWorldRef.Load(uint3(uv, 0)).xyz;
  uint2 curUV = uint2(0, 0);
  float disSq = 10000.0f;
  // 遍历整个场景图
  [loop]for (uint i = 0; i < 20; ++i) {
    [loop]for (uint j = 0; j < 20; ++j) {
      float4 tarPos = SMWorldPos.Load(uint3(i, j, 0));
      if (tarPos.a < 0.5f) continue;
      float3 dist = tarPos.xyz - oriPos;
      if (dot(dist, oriDir) < 0) continue;
      float curDis = dot(dist, dist);
      if (curDis > disSq) continue;
      dist = normalize(dist);
      if (dot(dist, oriDir) < 0.9) continue;
      disSq = curDis;
      curUV = uint2(i, j);
    }
  }
  RSAlbedo[uv] = SMAlbedo.Load(uint3(curUV, 0));
}
