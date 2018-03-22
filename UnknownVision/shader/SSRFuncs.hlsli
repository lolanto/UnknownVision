#include "PS_INPUT.hlsli"

#ifndef SSRFUNCS
#define SSRFUNCS

static const float maxDistance = 60.0f;
static const float initStride = 8.0f;
static const float maxSteps = 30.0f;

// clip the coordinate to [0,1]
void clipCoordinate(in float2 p0, inout float2 p1) {

  if (abs(p1.x - p0.x) < 0.00001f) {
    clamp(p1.y, 0, 1);
    return;
  }

  float k = (p1.y - p0.y) / (p1.x - p0.x);
  if (abs(k) < 0.00001f) {
    clamp(p1.x, 0, 1);
    return;
  }

  // p1.x E [0, 1]
  if (p1.x > 1) {
    p1.y = p1.y + k - k * p1.x;
    p1.x = 1;
  } else if (p1.x < 0) {
    p1.y = p0.y - k * p0.x;
    p1.x = 0;
  }

  // p1.y E [0, 1]
  if (p1.y > 1) {
    p1.x = p1.x + (1 - p1.y) / k;
    p1.y = 1;
  } else if (p1.y < 0) {
    p1.x = p1.x - (p1.y / k);
    p1.y = 0;
  }

  return;
}

float3 reconstCSPos(float2 uv, Texture2D<float> depthMap) {
  float4 cs = float4(uv, 0.0, 0.0);
  cs.z = depthMap.Load(int3(uv * GCameraParam.zw, 0)).r;
  float nf = GCameraParam.x * GCameraParam.y;
  float nminusf = GCameraParam.x - GCameraParam.y;
  cs.w = nf / (nminusf * (cs.z + GCameraParam.y / nminusf));
  cs.xy = cs.xy * 2 - 1;
  cs.y = -cs.y;
  cs.xyz *= cs.w;
  return mul(GProjectMatrixInv, cs).xyz;
}

float3 reconstCSPos2(float2 uv, float linearZ) {
  float4 cs = float4(uv, 0, linearZ);
  float nf = GCameraParam.x * GCameraParam.y;
  float nminusf = GCameraParam.x - GCameraParam.y;
  cs.z = nf / nminusf - linearZ * (GCameraParam.y / nminusf);
  cs.x = (cs.x * 2 - 1) * linearZ;
  cs.y = (cs.y * -2 + 1) * linearZ;
  return mul(GProjectMatrixInv, cs).xyz;
}

void swap (inout float a, inout float b) {
  float t = a;
  a = b;
  b = t;
}

float distanceSquared(float2 a, float2 b) {
  a -= b;
  return dot(a, a);
}

// 将深度图中的深度值还原为线性空间中的深度值
// uv: 像素单位，非UV
float linearDepthTexelFetch(float2 uv, Texture2D<float> depthMap) {
	float dv = depthMap.Load(int3(uv, 0)).r;
	float nf = GCameraParam.x * GCameraParam.y;
	float nminusf = GCameraParam.x - GCameraParam.y;
  return nf / (nminusf * (dv + GCameraParam.y / nminusf));
}

float linearDepthTexelFetch2(float2 uv, Texture2D depthValueMap) {
  float dv = depthValueMap.Load(int3(uv, 0)).a;
  return dv;
}

bool GreaterThan(float a, float b) {
  a -= b;
  return a > 0.000005;
}

bool intersectsDepthMap(float sceneZMax, float minZ, float maxZ) {
  // bool rel = (maxZ > sceneZMax) && (sceneZMax > minZ);
  bool rel = GreaterThan(maxZ, sceneZMax) && GreaterThan(sceneZMax, minZ);
  return rel;
}

bool traceScreenSpaceRay(
  float3 csOrig,
  float3 csDir,
  Texture2D depthMap,
  out float2 hitPixel) {
  hitPixel = float2(0, 0);
  float rayLength = ((csOrig.z + csDir.z * maxDistance) < GCameraParam.x) ?
  (GCameraParam.x - csOrig.z) / csDir.z : maxDistance;
  float3 csEndPoint = csOrig + csDir * rayLength;

  float4 H0 = mul(GProjectMatrix, float4(csOrig, 1.0f));
  H0.xy *= GCameraParam.zw;
  float4 H1 = mul(GProjectMatrix, float4(csEndPoint, 1.0f));
  H1.xy *= GCameraParam.zw;
  float k0 = 1.0f / H0.w;
  float k1 = 1.0f / H1.w;

  float2 p0 = (H0.xy * k0 + GCameraParam.zw) / 2;
  p0.y = GCameraParam.w - p0.y;
  float2 p1 = (H1.xy * k1 + GCameraParam.zw) / 2;
  p1.y = GCameraParam.w - p1.y;

  // 裁减终点位置使其位于屏幕内
  // p1.x = clamp(p1.x, 0, GCameraParam.z);
  // p1.y = clamp(p1.y, 0, GCameraParam.w);
  // hitPixel = float2(0, 0);
  // return p1.xy;

  p1 += (distanceSquared(p0, p1) < 0.0001f) ? float2(0.01f, 0.01f) : 0.0f;
  float2 delta = p1 - p0;

  bool permute = false;
  if (abs(delta.x) < abs(delta.y)) {
    permute = true;
    delta = delta.yx;
    p0 = p0.yx;
    p1 = p1.yx;
  }

  float stepDir = sign(delta.x);
  float invdx = stepDir / delta.x;

  float dk = (k1 - k0) * invdx;
  float2 dp = float2(stepDir, delta.y * invdx);

  // dp *= initStride;
  // dk *= initStride;

  float3 Pk = float3(p0, k0);
  float3 dPk = float3(dp, dk);

  float end = p1.x * stepDir;

  float stepCount = 0.0f;
  float prevZMaxEstimate = csOrig.z;
  float rayZMin = prevZMaxEstimate;
  float rayZMax = prevZMaxEstimate;

  float curRayZ = csOrig.z;
  float curStride = initStride;

  Pk += dPk * curStride;

  float sceneZMax = rayZMax + 100.0f;
  bool apprx = false;
  for(;
    ((Pk.x * stepDir) < end) && (stepCount < maxSteps) &&
  /*!intersectsDepthMap(sceneZMax, rayZMin, rayZMax) &&*/
  (sceneZMax != GCameraParam.y);
  ++stepCount) {
    // rayZMin = prevZMaxEstimate;
    // rayZMax = 1 / (Pk.z + dPk.z);
    // prevZMaxEstimate = rayZMax;
    // curRayZ = 1 / (Pk.z + dPk.z);
    curRayZ = 1 / (Pk.z);

    // if (rayZMin > rayZMax) {
    //   swap(rayZMin, rayZMax);
    // }

    hitPixel = permute ? Pk.yx : Pk.xy;
    // sceneZMax = linearDepthTexelFetch(hitPixel, depthMap);
    sceneZMax = linearDepthTexelFetch2(hitPixel, depthMap);
    // float offset = -dot(depthMap.Load(int3(hitPixel, 0)).xyz, csDir);
    if (curRayZ > sceneZMax) {
      if (curRayZ - sceneZMax < 0.07f) return true;
      curStride /= 2.0f;
      Pk -= dPk * curStride;
      apprx = true;
    } else {
      curStride = apprx ? curStride / 2.0f : curStride * 2.0f;
      Pk += dPk * curStride;
    }
  }
  // if (stepCount > maxSteps - 1 && curStride < 0.001f && sceneZMax > GCameraParam.x) return true;
  return false;
  // return intersectsDepthMap(sceneZMax, rayZMin, rayZMax);
}

#endif
