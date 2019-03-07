// 构建反射采样点基于当前反射方向的摄像机矩阵
Texture2D<float4> SSWPos : register(t0);
Texture2D<float4> SSWNor : register(t1);

struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4 vRef;
  float4 wNor;
  float4x4 refMatrix;
  float4x4 refProjMatrix;
};

RWStructuredBuffer<RefEleData> RefViewMatrixs : register(u0);

cbuffer ExtraData : register(b0) {
  // xy: screenSpace size; zw: select ref view point size
  float4 exData;
}

cbuffer CameraData : register(b1) {
  matrix GViewMatrix;
  matrix GViewMatrixInv;
  matrix GProjectMatrix;
  matrix GProjectMatrixInv;
  float4 GEyePos;
  // x: n, y: f, z: width, w: height
  float4 GCameraParam;
  /* 
  xy: nearPlaneSize.xy
  zw: farPlaneSize.xy
  */
  float4 GCameraParam2;
}

[numthreads(10, 10, 1)]
void main(uint3 GTID: SV_GroupThreadID,
  uint3 GID: SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;
  uint2 base = tuv * (exData.xy / exData.zw) + (exData.xy / exData.zw) / 2;
  
  float3 worldPos = SSWPos[base].xyz;
  float3 worldNor = normalize(SSWNor[base].xyz);
  float3 worldRef = reflect(normalize(worldPos - GEyePos.xyz), worldNor);

  // 反射开始点向反射方向偏移
  const float bias = 1.0f;
  worldPos += worldRef * bias;

  float3 up = float3(0.0f, 1.0f, 0.0f);
  if (abs(dot(up, worldRef)) > 0.9f) up.z += 0.1f;
  float3 right = normalize(cross(up, worldRef));
  up = cross(worldRef, right);

  Matrix m = {
    right   , dot(right   , -worldPos), // row1
    up      , dot(up      , -worldPos), // row2
    worldRef, dot(worldRef, -worldPos), // row3
    0.0f, 0.0f, 0.0f, 1.0f              // row4
  };

  RefViewMatrixs[tuv.x + tuv.y * exData.z].wPos = float4(worldPos, 1.0f);
  RefViewMatrixs[tuv.x + tuv.y * exData.z].wRef = float4(worldRef, 0.0f);
  RefViewMatrixs[tuv.x + tuv.y * exData.z].refMatrix = m;
}
