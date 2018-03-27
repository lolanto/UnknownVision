#include "../PS_INPUT.hlsli"


// 记录线性变换矩阵
Texture2D ltc_mat : register (t0);

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

struct Rect{
  float3 center; // 正方形在世界空间中的中心位置
  float3 dirx;  // 正方形的横边在世界空间中的方向
  float3 diry;  // 正方形的竖边在世界空间中的防线
  float2 halfxy;  // 正方形的长宽

  float4 plane;
};

struct Ray{
  float3 origin; // 光线的起点
  float3 dir;    // 光线的方向
};

const static float pi = 3.1415926f;

void ClipQuadToHorizon(inout float3 L[5], inout int n) {
// detect clipping config
  int config = 0;
  if (L[0].z > 0.0) config += 1;
  if (L[1].z > 0.0) config += 2;
  if (L[2].z > 0.0) config += 4;
  if (L[3].z > 0.0) config += 8;

    // clip
  n = 0;

  if (config == 0)
  {
      // clip all
  }
  else if (config == 1) // V1 clip V2 V3 V4
  {
      n = 3;
      L[1] = -L[1].z * L[0] + L[0].z * L[1];
      L[2] = -L[3].z * L[0] + L[0].z * L[3];
  }
  else if (config == 2) // V2 clip V1 V3 V4
  {
      n = 3;
      L[0] = -L[0].z * L[1] + L[1].z * L[0];
      L[2] = -L[2].z * L[1] + L[1].z * L[2];
  }
  else if (config == 3) // V1 V2 clip V3 V4
  {
      n = 4;
      L[2] = -L[2].z * L[1] + L[1].z * L[2];
      L[3] = -L[3].z * L[0] + L[0].z * L[3];
  }
  else if (config == 4) // V3 clip V1 V2 V4
  {
      n = 3;
      L[0] = -L[3].z * L[2] + L[2].z * L[3];
      L[1] = -L[1].z * L[2] + L[2].z * L[1];
  }
  else if (config == 5) // V1 V3 clip V2 V4) impossible
  {
      n = 0;
  }
  else if (config == 6) // V2 V3 clip V1 V4
  {
      n = 4;
      L[0] = -L[0].z * L[1] + L[1].z * L[0];
      L[3] = -L[3].z * L[2] + L[2].z * L[3];
  }
  else if (config == 7) // V1 V2 V3 clip V4
  {
      n = 5;
      L[4] = -L[3].z * L[0] + L[0].z * L[3];
      L[3] = -L[3].z * L[2] + L[2].z * L[3];
  }
  else if (config == 8) // V4 clip V1 V2 V3
  {
      n = 3;
      L[0] = -L[0].z * L[3] + L[3].z * L[0];
      L[1] = -L[2].z * L[3] + L[3].z * L[2];
      L[2] =  L[3];
  }
  else if (config == 9) // V1 V4 clip V2 V3
  {
      n = 4;
      L[1] = -L[1].z * L[0] + L[0].z * L[1];
      L[2] = -L[2].z * L[3] + L[3].z * L[2];
  }
  else if (config == 10) // V2 V4 clip V1 V3) impossible
  {
      n = 0;
  }
  else if (config == 11) // V1 V2 V4 clip V3
  {
      n = 5;
      L[4] = L[3];
      L[3] = -L[2].z * L[3] + L[3].z * L[2];
      L[2] = -L[2].z * L[1] + L[1].z * L[2];
  }
  else if (config == 12) // V3 V4 clip V1 V2
  {
      n = 4;
      L[1] = -L[1].z * L[2] + L[2].z * L[1];
      L[0] = -L[0].z * L[3] + L[3].z * L[0];
  }
  else if (config == 13) // V1 V3 V4 clip V2
  {
      n = 5;
      L[4] = L[3];
      L[3] = L[2];
      L[2] = -L[1].z * L[2] + L[2].z * L[1];
      L[1] = -L[1].z * L[0] + L[0].z * L[1];
  }
  else if (config == 14) // V2 V3 V4 clip V1
  {
      n = 5;
      L[4] = -L[0].z * L[3] + L[3].z * L[0];
      L[0] = -L[0].z * L[1] + L[1].z * L[0];
  }
  else if (config == 15) // V1 V2 V3 V4
  {
      n = 4;
  }
  
  if (n == 3)
      L[3] = L[0];
  if (n == 4)
      L[4] = L[0];
}

// 边积分
float IntegrateEdge(float3 v1, float3 v2) {
  float cosTheta = dot(v1, v2);
  float theta = acos(cosTheta);
  float res = cross(v1, v2).z * ((theta > 0.001) ? theta / sin(theta) : 1);
  return res;
}

// 积分器
float3 LTC_Evaluate(
  float3 N, float3 V, float3 P, float3 points[4]
  ) {
  // 构造面发现位置的标准正交基
  float3 T1, T2;
  T1 = normalize(V - N * dot(V, N));
  T2 = cross(N, T1);

  float3x3 Minv = float3x3(T1, T2, N);

  float3 L[5];
  L[0] = mul(Minv, points[0] - P);
  L[1] = mul(Minv, points[1] - P);
  L[2] = mul(Minv, points[2] - P);
  L[3] = mul(Minv, points[3] - P);

  int n;
  ClipQuadToHorizon(L, n);
  if (n == 0)
    return float3(0, 0, 0);

  L[0] = normalize(L[0]);
  L[1] = normalize(L[1]);
  L[2] = normalize(L[2]);
  L[3] = normalize(L[3]);
  L[4] = normalize(L[4]);

  // 积分和
  float sum = 0.0f;
  sum += IntegrateEdge(L[0], L[1]);
  sum += IntegrateEdge(L[1], L[2]);
  sum += IntegrateEdge(L[2], L[3]);
  if (n >= 4)
    sum += IntegrateEdge(L[3], L[4]);
  if (n == 5)
    sum += IntegrateEdge(L[4], L[0]);

  return float3(sum, sum, sum);
}

bool RayPlaneIntersect(Ray ray, float4 plane, inout float t) {
  /*
  dot(plane, float4(ray.origin, 1.0f))
  计算光线起点到平面的垂直距离
  dot(plane.xyz, ray.dir)
  计算光线方向的垂直长度

  两者相除可以计算出点面之间沿光线方向的距离
  */
  t = -dot(plane, float4(ray.origin, 1.0f)) / dot(plane.xyz, ray.dir);
  return t > 0.0f;
}

// 产生从摄像机发射的光线
Ray GenerateCameraRay(float2 uv) {
  Ray ray;

  float2 xy = uv * 2.0f - 1.0f;
  xy.y = -xy.y;

  ray.dir = normalize(float3(xy, 2.0f));

  float focalDistance = 2.0f; // 焦距
  float ft = focalDistance / ray.dir.z;
  float3 pFocus = ray.dir * ft;

  ray.origin = float3(0, 0, 0);
  ray.dir = normalize(pFocus - ray.origin);

  ray.origin = mul(GViewMatrix, float4(ray.origin, 1.0f)).xyz;
  ray.dir = mul(GViewMatrix, float4(ray.dir, 0.0f)).xyz;

  return ray;
}

// 生成固定的正方形
// 中心在世界坐标(0, 6, 2)
// 横竖对应x,y轴
// 宽高均为1
void InitRect(inout Rect rect) {

  rect.dirx = float3(1.0f, 0.0f, 0.0f);
  rect.diry = float3(0.0f, 1.0f, 0.0f);

  rect.center = float3(0.0f, 6.0f, 2.0f);
  rect.halfxy = float2(0.5f, 0.5f);

  float3 rectNor = cross(rect.diry, rect.dirx);
  rect.plane = float4(rectNor, -dot(rectNor, rect.center));
}

bool RayRectIntersect(Ray ray, Rect rect, out float t) {
  bool intersect = RayPlaneIntersect(ray, rect.plane, t);
  if (intersect) {
    float3 pos = ray.origin + ray.dir * t;
    float3 lpos = pos - rect.center;

    float x = dot(lpos, rect.dirx);
    float y = dot(lpos, rect.diry);

    if (abs(x) > rect.halfxy.x || abs(y) > rect.halfxy.y)
      intersect = false;
  }
  return intersect;
}

// 计算正方形的四个角点在空间中的位置
void InitRectPoints(Rect rect, inout float3 points[4]) {
  // 横竖方向的半边长向量
  float3 ex = rect.halfxy.x * rect.dirx;
  float3 ey = rect.halfxy.y * rect.diry;

  points[0] = rect.center - ex - ey; // 左下角
  points[1] = rect.center + ex - ey; // 右下角
  points[2] = rect.center + ex + ey; // 右上角
  points[3] = rect.center - ex + ey; // 左上角
}

float4 main(VSOutput i) : SV_Target {

  Rect rect;
  InitRect(rect);

  float3 points[4]; // 正方形的四个角点在空间中的位置
  InitRectPoints(rect, points);

  float4 floorPlane = float4(0, 1, 0, 0);

  float3 lcol = float3(0.8, 0.8, 0.8); // 灯光强度
  float3 dcol = float3(0.2, 0.2, 0.2); // 灯光diffuse颜色
  float3 scol = float3(0.4, 0.4, 0.4); // 灯光specular颜色

  float3 col = float3(0, 0, 0); // 最终输出的颜色

  Ray ray = GenerateCameraRay(i.uv);

  float distToFloor; // 摄像机到地面的距离
  bool hitFloor = RayPlaneIntersect(ray, floorPlane, distToFloor);
  if (hitFloor) {
    // 光线与平面的交点
    float3 pos = ray.origin + ray.dir * distToFloor;
    // 平面的法线
    float3 N = floorPlane.xyz;
    // 光线出射方向，指向视点
    float3 V = -ray.dir;

    // // 视线与平面的角度
    // float theta = acos(dot(N, V));

    // // 根据观察角度以及表面粗糙度，索引线性变换矩阵
    // float2 uv = float2(roughness, theta / (0.5f * pi));
    // // 适当便宜
    // uv = uv * LUT_SCALE + LUT_BIAS;

    // float4 t = ltc_mat.Load(uint3(uv, 0));
    // float3x3 Minv = float3x3(
    //     float3(1, 0, t.y),
    //     float3(0, t.z, 0),
    //     float3(t.w, 0, t.x)
    //   );

    float3 diff = LTC_Evaluate(N, V, pos, points);
    col = lcol * diff * dcol;
    col /= 2.0 * pi;
  }

  float distToRect;
  if (RayRectIntersect(ray, rect, distToRect))
    if ((distToRect < distToFloor) || !hitFloor)
      col = lcol;

  return float4(col, 1.0f);
}