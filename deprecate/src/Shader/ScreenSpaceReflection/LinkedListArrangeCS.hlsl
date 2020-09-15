#include "../PS_INPUT.hlsli"
#include "FormatConvert.hlsli"
#include "Filter.hlsli"


Texture2D<unorm float4> fragmentColor : register (t0);
Texture2D<uint>         fragmentHead  : register (t1);
Texture2D<uint2>        fragmentLink  : register (t2);
Texture2D<float4>       fragmentNor   : register (t3);
Texture2D<float4>       fragmentPos   : register (t4);

RWTexture2D<unorm float4> fragmentDiffuse : register (u0);
RWTexture2D<float2> fragmentDepth : register (u1);

// 最后输出时候的层数
#define NUM_LAYER 6
// 层级最大缓冲
#define NUM_CANDIDATE 6


cbuffer LinkedListData : register (b1) {
  // xy: buffer width height, z: storage, w: screen height
  float4 ScreenWidthHeightStorageSlice;
}

uint2 GetAddress(uint addr) {
  int2 re;
  re.x = int(ScreenWidthHeightStorageSlice.x);
  re.y = addr / re.x;
  re.x = addr % re.x;
  return re;
}

struct NodeData {
  float3 color;
  float3 normal;
  float3 position;
  float depth;
};

[numthreads(10, 10, 1)]
void main (uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {

  uint2 tuv = GID.xy * 10 + GTID.xy;
  uint next = fragmentHead.Load(uint3(tuv, 0));
  NodeData datas[NUM_CANDIDATE];
  uint2 puv = GetAddress(next);
  uint layer = 0;

  for (;

    (puv.x < ScreenWidthHeightStorageSlice.x &&
    puv.y < ScreenWidthHeightStorageSlice.y);

    puv = GetAddress(next),
    ++layer
  ) {
    datas[layer].color = fragmentColor.Load(uint3(puv, 0)).xyz;
    uint2 link = fragmentLink.Load(uint3(puv, 0));
    next = link.y;

    const float f = 1 / float(uint(0xffffffff));
    datas[layer].depth = float(link.x) * f;

    datas[layer].normal = fragmentNor.Load(uint3(puv, 0)).xyz;
    datas[layer].position = fragmentPos.Load(uint3(puv, 0)).xyz;
  }

  // sort!
  for (uint curLayer = 0; curLayer < layer; ++curLayer) {
    float minDepth = datas[curLayer].depth;
    uint minLayer = curLayer;
    for (uint nextLayer = curLayer + 1; nextLayer < layer; ++nextLayer) {
      float curDepth = datas[nextLayer].depth;
      if (curDepth < minDepth) {
        minDepth = curDepth;
        minLayer = nextLayer;
      }
    }
    NodeData temp = datas[curLayer];
    datas[curLayer] = datas[minLayer];
    datas[minLayer] = temp;
  }

  // for calc depth, corner.depth = 1
  float3 corner[4];
  corner[0].xy = tuv + float2(0.0f, 0.0f);
  corner[0].xy /= float2(ScreenWidthHeightStorageSlice.xw);
  corner[0].y = 1.0f - corner[0].y;
  corner[0].xy = (corner[0].xy * 2.0f - 1.0f);

  corner[1].xy = tuv + float2(1.0f, 0.0f);
  corner[1].xy /= float2(ScreenWidthHeightStorageSlice.xw);
  corner[1].y = 1.0f - corner[1].y;
  corner[1].xy = (corner[1].xy * 2.0f - 1.0f);

  corner[2].xy = tuv + float2(.0f, 1.0f);
  corner[2].xy /= float2(ScreenWidthHeightStorageSlice.xw);
  corner[2].y = 1.0f - corner[2].y;
  corner[2].xy = (corner[2].xy * 2.0f - 1.0f);

  corner[3].xy = tuv + float2(1.0f, 1.0f);
  corner[3].xy /= float2(ScreenWidthHeightStorageSlice.xw);
  corner[3].y = 1.0f - corner[3].y;
  corner[3].xy = (corner[3].xy * 2.0f - 1.0f);

  float depthInClip = (GCameraParam.y / (GCameraParam.x - GCameraParam.y)) * (GCameraParam.x - 1);
  corner[0] = normalize(mul(GProjectMatrixInv, float4(corner[0].xy, depthInClip, 1.0f)).xyz);
  corner[1] = normalize(mul(GProjectMatrixInv, float4(corner[1].xy, depthInClip, 1.0f)).xyz);
  corner[2] = normalize(mul(GProjectMatrixInv, float4(corner[2].xy, depthInClip, 1.0f)).xyz);
  corner[3] = normalize(mul(GProjectMatrixInv, float4(corner[3].xy, depthInClip, 1.0f)).xyz);
  // write back the result!
  for(uint i = 0; i < layer && i < NUM_LAYER; 
    ++i, tuv.y += int(ScreenWidthHeightStorageSlice.w)) {
    fragmentDiffuse[tuv] = float4(datas[i].color, 1.0f);
    // calc depth
    float scale = abs(dot(normalize(datas[i].position), datas[i].normal));
    // scale = pow(scale, 2);
    scale = 1 - pow(2.7182818f,-8 * scale);
    float pDotn = dot(datas[i].position, datas[i].normal);
    float z[4];
    z[0] = pDotn / dot(datas[i].normal, corner[0]);
    z[0] = z[0] * corner[0].z;
    z[1] = pDotn / dot(datas[i].normal, corner[1]);
    z[1] = z[1] * corner[1].z;
    z[2] = pDotn / dot(datas[i].normal, corner[2]);
    z[2] = z[2] * corner[2].z;
    z[3] = pDotn / dot(datas[i].normal, corner[3]);
    z[3] = z[3] * corner[3].z;
    float d_min = min(min(z[0], z[1]), min(z[2], z[3])) / GCameraParam.y;
    float d_max = max(max(z[0], z[1]), max(z[2], z[3])) / GCameraParam.y;

    d_max = datas[i].depth + (d_max - d_min) * scale;
    fragmentDepth[tuv] = float2(
      datas[i].depth, d_max);
      // datas[i].depth,
      // datas[i].depth + epsilon(datas[i].depth, 0));
      // d_max / GCameraParam.y);
      // colorAndDepth[i].w + 0.002f);
  }

  return;
}