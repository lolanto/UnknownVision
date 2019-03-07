#include "../PS_INPUT.hlsli"

#ifndef SCREEN_SPACE_RAY_TRACING
#define SCREEN_SPACE_RAY_TRACING

// 定义的外部资源
SamplerState SSRT_PointSampler : register (s0);

// 包含光线追踪相关函数
// 计算向量距离的平方
float distanceSquared(float2 a, float2 b);
// 交换数值
void swap(inout float a, inout float b);
// 输入非线性深度缓冲中的值重构摄像机空间(线性)的深度值
float reconstructZValue(float v);
// 输入屏幕uv坐标以及深度图，还原摄像机空间坐标
float3 reconstructCSPos(float2 uv, Texture2D csZBuffer,
    bool csZBufferIsHyperbolic);
// 光线追踪主函数
bool traceScreenSpaceRay(
  float3          csOrigin,
  float3          csDirection,
  Texture2D       csZBuffer, // 摄像机空间下的深度缓冲区
  bool            csZBufferIsHyperbolic, // 深度缓冲中的值是否为非线性
  float           csZThickness,
  float           stride,
  float           jitterFraction,
  float           maxSteps,
  in float        maxDistance,
  out float2      hitPixel,
  out float       resType);


float distanceSquared(float2 a, float2 b) {
  a = a - b;
  return dot(a, a);
}

void swap(inout float a, inout float b) {
  float c = a;
  a = b;
  b = c;
}

float reconstructZValue(float v) {
  static float a = GCameraParam.x * GCameraParam.y;
  static float b = GCameraParam.x - GCameraParam.y;
  static float c = GCameraParam.y / b;
  return a / ((v + c) * b);
}

float3 reconstructCSPos(float2 uv, Texture2D csZBuffer,
    bool csZBufferIsHyperbolic) {
  float4 ndc = float4(uv, 0, 0);
  ndc.w = csZBufferIsHyperbolic ? 
    reconstructZValue(csZBuffer.Sample(SSRT_PointSampler, uv).r ) : csZBuffer.Sample(SSRT_PointSampler, uv).r;
  ndc.xy = ((ndc.xy * 2.0f) - 1.0f) * ndc.w;
  ndc.y = -ndc.y;
  float nf = GCameraParam.x * GCameraParam.y;
  float nSubf = GCameraParam.x - GCameraParam.y;
  ndc.z = (nf / nSubf) - GCameraParam.y * ndc.w / nSubf;
  ndc = mul(GProjectMatrixInv, ndc);
  return ndc.xyz;
}

bool traceScreenSpaceRay(
  float3          csOrigin,
  float3          csDirection,
  Texture2D       csZBuffer, // 摄像机空间下的深度缓冲区
  bool            csZBufferIsHyperbolic, // 深度缓冲中的值是否为非线性
  float           csZThickness,
  float           stride,
  float           jitterFraction,
  float           maxSteps,
  in float        maxDistance,
  out float2      hitPixel,
  out float       resType) {

  // 光线的最大长度
  float rayLength = ((csOrigin.z + csDirection.z * maxDistance) < GCameraParam.x) ?
                      (GCameraParam.x - csOrigin.z) / csDirection.z :
                      maxDistance;

  // 光线的端点
  float3 csEndPoint = csOrigin + csDirection * rayLength;

  // 将端点投影到裁剪空间
  float4 H0 = mul(GProjectMatrix, float4(csOrigin, 1.0f));
  float4 H1 = mul(GProjectMatrix, float4(csEndPoint, 1.0f));

  // 试图将H1限制在裁剪空间中
  // abs(x,y,z) <= w
  if (abs(H1.x) > H1.w) {
    float a = sign(H1.x);
    float numerator = a * H1.w - H1.x;
    float denominator = H0.x - a * H0.w ? H0.x - a * H0.w : 0.001f;
    float t = 1 / (numerator / denominator + 1);
    H1 = (1 - t) * H0 + t * H1;
  }

  if (abs(H1.y) > H1.w) {
    float a = sign(H1.y);
    float numerator = a * H1.w - H1.y;
    float denominator = H0.y - a * H0.w ? H0.y - a * H0.w : 0.001f;
    float t = 1 / (numerator / denominator + 1);
    H1 = (1 - t) * H0 + t * H1;
  }

  // 深度值的倒数(可在屏幕空间插值)
  float k0 = 1.0f / H0.w;
  float k1 = 1.0f / H1.w;

  // 将xy转换到NDC中
  float2 P0 = H0.xy * k0;
  float2 P1 = H1.xy * k1;

  // 初始化输出结果
  hitPixel = float2(-1.0f, -1.0f); // 左上角

  // 保证光线投影到NDC中的距离不会过小而出现除以0
  P1 += distanceSquared(P0, P1) < 0.0001f ? float2(0.01f, 0.01f) : float2(0, 0);

  float2 delta = P1 - P0;

  bool permute = false;
  if (abs(delta.x) < abs(delta.y)) {
    // 投影线段更垂直
    permute = true;
    delta = delta.yx;
    P0 = P0.yx; P1 = P1.yx;
  }

  float stepDirection = sign(delta.x);
  float invdx = stepDirection / delta.x;
  // 单位步长移动的距离
  float2 dP = float2(stepDirection, invdx * delta.y);

  float dk = (k1 - k0) * invdx;

  // 应用步长
  dP *= stride; dk *= stride;

  // 添加抖动
  P0 += dP * jitterFraction; k0 += dk * jitterFraction;

  float k = k0;

  float stepCount = 0.0f;
  float curRayZ = csOrigin.z;
  float sceneZ = curRayZ + 1e4;

  float end = P1.x * stepDirection;

  for (float2 P = P0;
    ((P.x * stepDirection) <= end) && // 当前点未超过末端点
    (stepCount < maxSteps) && // 仍然有迭代次数
    ((curRayZ < sceneZ) || // 光线在表面前面
      (curRayZ > sceneZ + csZThickness)) && // 光线在表面背面但超过像素厚度
    (sceneZ != 0.0f); // 光线尚未碰到近裁面
    P += dP, k += dk, stepCount += 1.0f) {

    hitPixel = permute ? P.yx : P;
    // P为光线段进行透视投影之后的结果，故视锥体内的取值范围为-1~1
    // 需要转换为uv空间
    hitPixel = (hitPixel + 1) / 2;
    hitPixel.y = 1.0f - hitPixel.y;

    // 计算光线在当前位置步进半步的深度值
    curRayZ = 1 / k;

    // 计算当前像素位置的深度值
    sceneZ = csZBuffer.Sample(SSRT_PointSampler, hitPixel).r;
    sceneZ = csZBufferIsHyperbolic ? reconstructZValue(sceneZ) : sceneZ;
  }

  if (stepCount >= maxSteps) resType = 0.0f;
  if (curRayZ >= sceneZ) {
    resType = 1.0f;
    if (curRayZ < sceneZ + csZThickness) resType = 2.0f;
  }

  return (curRayZ >= sceneZ && curRayZ < sceneZ + csZThickness);
}

#endif
