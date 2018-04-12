#include "FormatConvert.hlsli"
#include "Filter.hlsli"

Texture2D<unorm float4> fragmentColor : register (t0);
Texture2D<uint>         fragmentHead  : register (t1);
Texture2D<uint2>        fragmentLink  : register (t2);

RWTexture2D<unorm float4> fragmentDiffuse : register (u0);
RWTexture2D<float2> fragmentDepth : register (u1);

// 最后输出时候的层数
#define NUM_LAYER 6
// 层级最大缓冲
#define NUM_CANDIDATE 6


cbuffer LinkedListData : register (b1) {
  float4 ScreenWidthHeightStorageSlice;
}

uint2 GetAddress(uint addr) {
  int2 re;
  re.x = int(ScreenWidthHeightStorageSlice.x);
  re.y = addr / re.x;
  re.x = addr % re.x;
  return re;
}

[numthreads(10, 10, 1)]
void main (uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {

  uint2 tuv = GID.xy * 10 + GTID.xy;
  uint next = fragmentHead.Load(uint3(tuv, 0));
  uint2 link;
  float4 colorAndDepth[NUM_CANDIDATE];
  uint2 puv;

  uint time;
  for (
    time = 0,
    puv = GetAddress(next);

    (next > 0 &&
    puv.x < ScreenWidthHeightStorageSlice.x &&
    puv.y < ScreenWidthHeightStorageSlice.y);

    tuv.y += int(ScreenWidthHeightStorageSlice.w),
    puv = GetAddress(next),
    ++time
  ) {
    colorAndDepth[time].xyz = fragmentColor.Load(uint3(puv, 0)).xyz;
    link = fragmentLink.Load(uint3(puv, 0));
    next = link.y;
    const float f = 1 / float(uint(0x0fffffff));

    colorAndDepth[time].w = float(link.x) * f;
  }

  // sort!
  for (uint curLayer = 0; curLayer < time; ++curLayer) {
    float minDepth = colorAndDepth[curLayer].w;
    uint minLayer = curLayer;
    for (uint nextLayer = curLayer + 1; nextLayer < time; ++nextLayer) {
      float curDepth = colorAndDepth[nextLayer].w;
      if (curDepth < minDepth) {
        minDepth = curDepth;
        minLayer = nextLayer;
      }
    }
    float4 temp = colorAndDepth[curLayer];
    colorAndDepth[curLayer] = colorAndDepth[minLayer];
    colorAndDepth[minLayer] = temp;
  }

  tuv = GID.xy * 10 + GTID.xy;
  for(uint i = 0; i < time && i < NUM_LAYER; ++i, tuv.y += int(ScreenWidthHeightStorageSlice.w)) {
    // colorAndDepth[i].w /= 10000000.0f;
    // //将深度变成线性分布
    // float csDepth = ((GCameraParam.x * GCameraParam.y)) /
    //   ((GCameraParam.x - GCameraParam.y) * colorAndDepth[i].w + GCameraParam.y);
    // colorAndDepth[i].w = (csDepth - GCameraParam.x) / (GCameraParam.y - GCameraParam.x);

    fragmentDiffuse[tuv] = float4(colorAndDepth[i].xyz, 1.0f);
    fragmentDepth[tuv] = float2(
      colorAndDepth[i].w,
      colorAndDepth[i].w + epsilon(colorAndDepth[i].w, 0));
  }

  return;
}