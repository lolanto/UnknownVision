#include "../PS_INPUT.hlsli"

#define EPSILON 0.0001f
// #define EPSILON 0.001f
#define MAX_STEPS 200
#define NUM_LAYER 6
#define LEVELS 6

Texture2D SSNor : register(t0);
Texture2D SSPos : register(t1);

Texture2D<float4> BasicDiffuse : register(t2);
Texture2D<float2> Level0 : register(t3);
Texture2D<float2> Level1 : register(t4);
Texture2D<float2> Level2 : register(t5);
Texture2D<float2> Level3 : register(t6);
Texture2D<float2> Level4 : register(t7);
Texture2D<float2> Level5 : register(t8);
Texture2D<float2> Level6 : register(t9);

RWTexture2D<unorm float4> result : register(u0);

struct Ray {
  float3 orig;
  float3 dir;
};

struct RayTracingState {
  float2 P0, dP;
  float k0, dk;
  // 当前插值步长
  float t;
  uint curLevel;
  uint layer;
  float2 pixlen;
  float2 posDir;
  bool done;
};

/*
将屏幕空间中的点投影回摄像机空间
sspos: 屏幕空间的点，单位是像素
depth: 点在摄像机空间中的深度值
*/
float3 reproject(float2 sspos, float depth = 1.0f);
// 输入反射光线，初始化追踪状态
void setupState(Ray ray,inout RayTracingState state);
// 根据全局变量state 获得当前点的屏幕空间坐标
float2 getCurrSSPos(RayTracingState state);
// 根据当前MipLevel以及UV坐标，获得深度信息
float2 getLevelData(uint level, uint2 uv);

float distanceSquared(float2 a, float2 b) {
  a = a - b;
  return dot(a, a);
}

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  // tuv起点像素位置
  uint2 tuv = GID.xy * 10 + GTID.xy;
  // 起点法线,位置以及反射方向
  float3 csNor = normalize(SSNor.Load(uint3(tuv, 0)).xyz);
  float3 csPos = SSPos.Load(uint3(tuv, 0)).xyz;
  float3 csRef = normalize(reflect(
    normalize(csPos), csNor));
  Ray refRay;
  refRay.orig = csPos;
  refRay.dir = csRef;

  RayTracingState state;

  setupState(refRay, state);

  float acceleration_delay = 1;
  // star ray marching
  for (int i = 0; i < MAX_STEPS && state.t < 1.0f; ++i) {
    // init
    float div = int(1 << state.curLevel);
    float2 Pcurr = getCurrSSPos(state);
    // 当前level下应该采样的像素位置
    uint2 levelUV = uint2(Pcurr / div);
    // 计算光线和当前像素边界的两个碰撞点
    float2 dt_entry = abs(ceil(Pcurr/div - state.posDir) * div - state.P0) * state.pixlen;
    float2 dt_exit = abs(floor(Pcurr/div + state.posDir) * div - state.P0) * state.pixlen;
    float t_entry = clamp(max(dt_entry.x, dt_entry.y), 0, state.t);
    float t_exit = clamp(min(dt_exit.x, dt_exit.y), t_entry, 1);
    // 计算当前的光线与像素边界碰撞点的深度值(注意深度值的分布)
    // k的值是摄像机深度的倒数
    float rayEntryDepth = state.k0 + state.dk * t_entry;
    rayEntryDepth = 1.0f / (rayEntryDepth * GCameraParam.y);

    float rayExitDepth = state.k0 + state.dk * t_exit;
    rayExitDepth = 1.0f / (rayExitDepth * GCameraParam.y);

    float rayDepthMin = min(rayEntryDepth, rayExitDepth);
    float rayDepthMax = max(rayEntryDepth, rayExitDepth);
    // 采样并判断碰撞结果
    bool collision = false;
    for (state.layer = 0;
      state.layer < NUM_LAYER;
      ++state.layer, levelUV.y += GCameraParam.w / div) {
      float2 sceneDepth = getLevelData(state.curLevel, levelUV);
      if (sceneDepth.x > rayDepthMax) break;
      if (sceneDepth.y < rayDepthMin) continue;
      collision = true;
      float f = clamp((sceneDepth.x - rayDepthMin) / (rayDepthMax - rayDepthMin), 0, 1);
      if (refRay.dir.z > 0)
        state.t = max(state.t, t_exit * f + t_entry * (1 - f));
      break;
    }

    // check if done
    if (collision && state.curLevel == 0) {
      state.done = true;
      break;
    }
    // update
    state.t = collision ? state.t : t_exit + EPSILON;
    // update current level
    acceleration_delay = max(0, collision ? 3 : acceleration_delay - 1);
    int new_level = clamp(state.curLevel + (collision ? -1 : 1), 0, LEVELS);
    state.curLevel = collision || acceleration_delay == 0 ? new_level : state.curLevel;
  }

  if (state.done) {
    uint2 pixel = getCurrSSPos(state);
    pixel.y = state.layer ? pixel.y + GCameraParam.w * state.layer : pixel.y;
    result[tuv] = BasicDiffuse.Load(uint3(pixel, 0));
  }
}


float3 reproject(float2 sspos, float depth) {
  // 构建NDC中的坐标 --> 裁剪空间坐标
  sspos /= GCameraParam.zw;
  sspos.y = 1.0f - sspos.y;
  sspos = (sspos * 2.0f - 1.0f) * depth;

  float4 rePos = float4(sspos, 
    (GCameraParam.y * (GCameraParam.x - depth)) / (GCameraParam.x - GCameraParam.y)
    , depth);

  rePos = mul(GProjectMatrixInv, rePos);
  return rePos.xyz;
}

void setupState(Ray ray,inout RayTracingState state) {
  float4 H0 = mul(GProjectMatrix, float4(ray.orig, 1.0f));
  state.k0 = 1.0f / H0.w;
  state.P0 = ((H0.xy * state.k0) + 1.0f) / 2.0f;
  state.P0.y = 1.0f - state.P0.y;
  state.P0 *= GCameraParam.zw;

  float2 P1;
  float4 Hclip = mul(GProjectMatrix, float4(ray.orig + ray.dir, 1.0f));
  float2 Pclip = ((Hclip.xy / Hclip.w + 1.0f) / 2.0f);
  Pclip.y = 1.0f - Pclip.y;
  Pclip = Pclip * GCameraParam.zw;
  Pclip += (distanceSquared(Pclip, state.P0) < 0.000001f) ? float2(0.001f, 0.001f) : float2(0, 0);
  float2 dir_screen = normalize(Pclip - state.P0);
  float2 dest_screen = step(0, dir_screen) * (GCameraParam.zw - uint2(1, 1));
  float2 dist_screen = (dest_screen - state.P0) / dir_screen;
  // P1为光线投影在屏幕上的边界位置
  P1 = state.P0 + dir_screen * min(dist_screen.x, dist_screen.y);

  // 将光线重新投影回摄像机空间
  float3 csFar = reproject(P1);
  float3 csFarDir = normalize(csFar);
  float3 n = cross(csFarDir, cross(ray.dir, csFarDir));
  float dirN = dot(ray.dir, n);
  dirN += dirN == 0 ? 0.0001f : 0;
  float len = dot(csFar - ray.orig, n) / dirN;
  float3 csEnd = ray.orig + ray.dir * len;

  if (csEnd.z > GCameraParam.y || len < 0)
    csEnd = ray.orig + ray.dir * ((GCameraParam.y - ray.orig.z) / ray.dir.z);
  if (csEnd.z < GCameraParam.x)
    csEnd = ray.orig + ray.dir * ((GCameraParam.x - ray.orig.z) / ray.dir.z);
  float4 H1 = mul(GProjectMatrix, float4(csEnd, 1.0f));
  float k1 = 1.0f / H1.w;
  P1 = ((H1 * k1 + 1.0f) / 2.0f);
  P1.y = 1.0f - P1.y;
  // 光线末端在屏幕上最终的投影位置
  P1 = P1 * GCameraParam.zw;

  // 初始化变量
  state.dP = P1 - state.P0;
  state.dk = k1 - state.k0;
  // xy方向上像素百分比
  state.pixlen = 1.0f / abs(state.dP);
  state.curLevel = 0;
  state.posDir = step(float2(0, 0), state.dP);
  // 初始化抖动
  float2 dt = abs(floor(state.P0 + state.posDir) - state.P0) * state.pixlen;
  state.t = min(dt.x, dt.y) + EPSILON;
  state.done = false;
  state.layer = 0;
}

float2 getCurrSSPos(RayTracingState state) {
  return state.P0 + state.t * state.dP;
}

float2 getLevelData(uint level, uint2 uv) {
  switch (level) {
  case 0:
    return Level0.Load(uint3(uv, 0)).xy;
  case 1:
    return Level1.Load(uint3(uv, 0)).xy;
  case 2:
    return Level2.Load(uint3(uv, 0)).xy;
  case 3:
    return Level3.Load(uint3(uv, 0)).xy;
  case 4:
    return Level4.Load(uint3(uv, 0)).xy;
  case 5:
    return Level5.Load(uint3(uv, 0)).xy;
  case 6:
    return Level6.Load(uint3(uv, 0)).xy;
  }
  return float2(-1, -1);
}