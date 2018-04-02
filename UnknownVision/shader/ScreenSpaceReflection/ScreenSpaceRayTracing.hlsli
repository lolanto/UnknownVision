float distanceSquared(float2 a, float b) {
  a = a - b;
  return dot(a, a);
}

void swap(inout float a, inout float b) {
  float c = a;
  a = b;
  b = a;
}

sampler2D SSRT_PointSampler {
  Filter = MIN_MAG_MIP_POINT,
  AddressU = Wrap,
  AddressV = Wrap,
  AddressW = Wrap,
  MaxLOD = 0,
  MinLOD = 0,
  MipLODBias = 0
};

bool traceScreenSpaceRay(
  float3          csOrigin,
  float3          csDirection,
  float4x4        proj,
  Texture2D       csZBuffer, // 摄像机空间下的深度缓冲区
  float           csZThickness,
  float           nearPlaneZ,
  float           stride,
  float           jitterFraction,
  float           maxSteps,
  in float        maxDistance,
  out float2      hitPixel,
  out float3      csHitPoint) {

  // 光线的最大长度
  float rayLength = ((csOrigin.z + csDirection.z * maxDistance) < nearPlaneZ) ?
                      (nearPlaneZ- csOrigin.z) / csDirection.z :
                      maxDistance;

  // 光线的端点
  float3 csEndPoint = csOrigin + csDirection * rayLength;

  // 将端点投影到裁剪空间
  float4 H0 = mul(proj, float4(csOrigin, 1.0f));
  float4 H1 = mul(proj, float4(csEndPoint, 1.0f));

  // 深度值的倒数(可在屏幕空间插值)
  float k0 = 1.0f / H0.w;
  float k1 = 1.0f / H1.W;

  // 对空间坐标进行插值(实际上就是投影面的坐标)
  float3 Q0 = csOrigin * k0;
  float3 Q1 = csEndPoint * k1;

  // 投影面坐标
  float2 P0 = H0.xy * k0;
  float2 P1 = H1.xy * k1;

  // 初始化输出结果
  hitPixel = float2(-1.0f, -1.0f); // 左上角


  // 保证光线段投影到投影面的长度至少一个像素大小
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

  float3 dQ = (Q1 - Q0) * invdx;
  float dk = (k1 - k0) * invdx;

  // 应用步长
  dP *= stride; dQ *= stride; dk *= stride;

  // 添加抖动
  P0 += dP * jitterFraction; Q0 += dQ * jitterFraction; k0 += dk * jitterFraction;

  float3 Q = Q0;
  float k = k0;

  float prevZMaxEstimate = csOrigin.z;
  float stepCount = 0.0f;
  float rayZMax = prevZMaxEstimate;
  float rayZMin = prevZMaxEstimate;
  float sceneZMax = rayZMax + 1e4;

  float end = P1.x * stepDirection;

  for (float2 P = P0;
    ((P.x * stepDirection) <= end) && // 当前点未超过末端点
    (stepCount < maxSteps) && // 仍然有迭代次数
    ((rayZMax < sceneZMax - csZThickness) || // 光线在体素前面
      (rayZMin > sceneZMax)) && // 光线在体素后面
    (sceneZMax != 0.0f); // 光线尚未碰到近裁面
    P += dP, Q.z += dQ.z, k += dk, stepCount += 1.0f) {

    hitPixel = permute ? P.yx : p;
    // P为光线段进行透视投影之后的结果，故视锥体内的取值范围为-1~1
    // 需要转换为uv空间
    hitPixel = (hitPixel + 1) / 2;
    hitPixel.y = 1.0f - hitPixel.y;
    // 将上次光线末端点位置存储到rayZMin中
    rayZMin = prevZMaxEstimate;

    // 计算光线在当前位置步进半步的深度值
    rayZMax = (dQ.z * 0.5f + Q.z) / (dk * 0.5f + k);
    // 将步进结果存储到prevZMaxEstimate中
    prevZMaxEstimate = rayZMax;
    // 保证Min为光线的近端(靠近摄像机), Max为光线的远端(远离摄像机)
    if (rayZMin > rayZMax) { swap(rayZMin, rayZMax); }



    // 计算当前像素位置的深度值
    sceneZMax = csZBuffer.Sample(SSRT_PointSampler, hitPixel).r;
  }

  Q.xy += dQ.xy * stepCount;
  csHitPoint = Q * (1.0f / k);

  // 当且仅当当前光线步进的两个端点均在当前像素体素范围内时才确认碰撞
  return (rayZMax >= sceneZMax - csZThickness) && (rayZMin <= sceneZMax);
}