#include "../PS_INPUT.hlsli"

static const float MaxDistance = 60.0f;
static const float MaxSteps = 30.0f;

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

struct PSOutput {
  float4 color1 : SV_Target0;
};

Texture2D normalAndLinearZ : register (t0);
Texture2D basicColor : register (t1);
SamplerState pointSampler : register (s0);
SamplerState linearSampler : register (s1);

float distanceSquare(float2 a, float2 b) {
  a -= b;
  return dot(a, a);
}

// 通过线性深度值以及UV坐标重构摄像机空间下的坐标
float3 reconsViewSpaceCoordinate(float2 uv) {
  float4 ndc = float4(uv, 0, normalAndLinearZ.Sample(pointSampler, uv).w);
  ndc.xy = ((ndc.xy * 2.0f) - 1.0f) * ndc.w;
  ndc.y = -ndc.y;
  float nf = GCameraParam.x * GCameraParam.y;
  float nSubf = GCameraParam.x - GCameraParam.y;
  ndc.z = (nf / nSubf) - GCameraParam.y * ndc.w / nSubf;
  ndc = mul(GProjectMatrixInv, ndc);
  return ndc.xyz;
}

bool SSR(float3 orgPnt, float2 uv, inout float2 hitUV) {
  float4 NrmAndLinearZ = normalAndLinearZ.Sample(pointSampler, uv);
  float3 csNor = normalize(NrmAndLinearZ.xyz);
  float3 csDir = normalize(reflect(orgPnt, csNor));
  float rayLength = ((orgPnt.z + csDir.z * MaxDistance) < GCameraParam.x) ?
    (GCameraParam.x - orgPnt.z) / csDir.z : MaxDistance;

  float3 csEndPoint = orgPnt + csDir * rayLength;
  float4 H = mul(GProjectMatrix, float4(csEndPoint, 1.0f));
  float k0 = 1 / NrmAndLinearZ.w;
  float k1 = 1 / H.w;

  float2 p0 = uv;
  float2 p1 = float2(H.xy * k1 / 2 + 0.5f);
  p1.y = 1.0f - p1.y;

  // 偏移之后，深度值也应该偏移，但这里没有进行
  // 可能会引入误差
  p1 += (distanceSquare(p0, p1) < 0.0000001f) ? float2(0.0001f, 0.0001f) : 0.0f;

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

  float3 pk = float3(p0, k0);
  float3 dpk = float3(dp, dk);

  float end = p1.x * stepDir;

  float stepCount = 0.0f;
  float rayZ = orgPnt.z;
  float sceneZ = orgPnt.z + GCameraParam.y;

  // 图像长宽的倒数，uv偏移的最小移动单位
  // 相当于一个像素的uv大小
  float2 curStride;
  normalAndLinearZ.GetDimensions(curStride.x, curStride.y);
  curStride = permute ? curStride.yx : curStride.xy;
  curStride = 1.0f / curStride;

  // marching的移动步幅
  float strideScale = 32.0f;
  // 当前是否正在逼近(二分查找)
  bool apprx = false;
  // 开始marching前先进行小幅度偏移，避免原地比较Z值冲突
  pk += dpk * curStride.x;
  for(;
    (pk.x * stepDir) < end &&
    (stepCount < MaxSteps); ++stepCount) {

    rayZ = 1 / pk.z;
    sceneZ = normalAndLinearZ.Sample(linearSampler, permute ? pk.yx : pk.xy).w;
    if (rayZ < sceneZ) {
      // 尚未碰到
      // 当前正在逼近，步幅为原来的一半且前进
      if (apprx) strideScale = abs(strideScale) * 0.5f;
    } else {
      // 可能碰到
      // 当前光线与高度场差距在阈值(0.1)内，认为已经碰撞
      if (rayZ - sceneZ < 0.005f) {
        hitUV = permute ? pk.yx : pk.xy;
        return true;
      }
      // 光线位置之前的某个位置已经与高度场相交，开始二分查找
      strideScale = -0.5f * abs(strideScale);
      apprx = true;
    }
    pk += dpk * curStride.x * strideScale;
  }
  return false;
}

PSOutput main (VSOutput i) {
  PSOutput output;
  float3 orgPnt = reconsViewSpaceCoordinate(i.uv);
  float2 hitPnt = float2(0, 0);
  if (SSR(orgPnt, i.uv, hitPnt)) output.color1 = basicColor.Sample(pointSampler, hitPnt);
  else output.color1 = float4(0, 0, 0, 1);
  return output;
}
