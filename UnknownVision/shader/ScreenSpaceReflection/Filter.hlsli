#include "../PS_INPUT.hlsli"
#ifndef FILETER
#define FILETER

#define epsilon_factor 2

float w(float depth, uint level) {
  const float base_radius = GCameraParam2.x / GCameraParam.z;
  const float add_radius = (GCameraParam2.z - GCameraParam2.x) / GCameraParam.z;
  return (base_radius + depth * add_radius) * float(1 << level);
}

// 计算d_min的深度偏移值, level是当前的层数
float epsilon(float d_min, uint level) {
  const float inv_far = 1.0f / GCameraParam.y;
  return epsilon_factor * inv_far * w(d_min, level + 1);
}

#endif