#include "Filter.hlsli"

// 最后输出时候的层数
#define NUM_LAYER 6

Texture2D<float2> fragmentDepthLastLevel : register(t0);

RWTexture2D<float2> fragmentDepthNextLevel : register(u0);

cbuffer LevelData : register (b1) {
  // x: current level, yz: levelSize, w: lastLevelSlice
  float4 LevelNumSizeLastSlice;
}

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {

  uint2 tuv = GID.xy * 10 + GTID.xy;
  if (tuv.x >= LevelNumSizeLastSlice.y || tuv.y >= LevelNumSizeLastSlice.z) return;
  uint2 base[4];
  float2 frags[4];
  base[0] = tuv * 2;
  base[1] = tuv * 2 + uint2(1, 0);
  base[2] = tuv * 2 + uint2(0, 1);
  base[3] = tuv * 2 + uint2(1, 1);

  frags[0] = fragmentDepthLastLevel.Load(uint3(base[0], 0));
  frags[1] = fragmentDepthLastLevel.Load(uint3(base[1], 0));
  frags[2] = fragmentDepthLastLevel.Load(uint3(base[2], 0));
  frags[3] = fragmentDepthLastLevel.Load(uint3(base[3], 0));
  float d_min = min(min(frags[0].x, frags[1].x), min(frags[2].x, frags[3].x));
  if (frags[0].x <= 0 && frags[1].x <= 0 && frags[2].x <= 0 && frags[3].x <= 0) return;
  for(uint cur_lay = 0; cur_lay < NUM_LAYER; ++cur_lay) {
    
    float d_max = d_min + epsilon(d_min, uint(LevelNumSizeLastSlice.x));
    float cur_max = 0;
    for(uint i = 0; i < 4; ++i) {
      while(frags[i].x > 0) {
        // 纳入当前层
        if (frags[i].y <= d_max) {
          cur_max = frags[i].y > cur_max ? frags[i].y : cur_max;
          base[i].y += LevelNumSizeLastSlice.w;
          frags[i] = fragmentDepthLastLevel.Load(uint3(base[i], 0));
        } else{
          break;
        }
      }
    }

    fragmentDepthNextLevel[tuv] = float2(d_min, cur_max);
    d_min = min(min(frags[0].x, frags[1].x), min(frags[2].x, frags[3].x));
    if (frags[0].x <= 0 && frags[1].x <= 0 && frags[2].x <= 0 && frags[3].x <= 0) break;
    tuv.y += LevelNumSizeLastSlice.z;
  }
}