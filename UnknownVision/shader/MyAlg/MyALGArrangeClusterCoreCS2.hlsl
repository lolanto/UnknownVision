Texture2D<uint> SSID : register(t0);

RWTexture2D<uint> SSIndex : register(u0);

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID: SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;
  int2 ScreenSize;
  SSID.GetDimensions(ScreenSize.x, ScreenSize.y);
  // 当前的面ID
  uint id = SSID[tuv];
  if (id == 0xffffffffu) return;
  // if (id > 10000000) return;
  // 当前所在的块
  uint2 tile = tuv / (ScreenSize / uint2(5, 5));
  uint2 slot = uint2((tile.x + tile.y * 5) * 36 + id, 0);
  SSIndex[slot] = 1;
}
