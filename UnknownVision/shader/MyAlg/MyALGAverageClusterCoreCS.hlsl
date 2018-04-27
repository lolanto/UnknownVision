Texture2D<float4> ClusterResultPos : register(t0);
Texture2D<float4> ClusterResultNor : register(t1);

cbuffer CameraData : register(b0) {
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

cbuffer RefPntData : register(b1) {
  // xy: Screen Width And Height; 
  //zw: Number of Reflection Sample point per Row and Column
  float4 RefPntData;
}

struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4x4 refMatrix;
};

RWStructuredBuffer<RefEleData> RefViewData : register(u0);

[numthreads(10, 10, 1)]
void main(uint3 GTID: SV_GroupThreadID,
  uint2 GID: SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;
  float4 CluPos = ClusterResultPos[tuv];
  float3 worldPos = CluPos.xyz / CluPos.w;

  float4 CluNor = ClusterResultNor[tuv];
  float3 worldNor = CluNor.xyz / CluNor.w;

  float3 worldRef = reflect(normalize(worldPos - GEyePos.xyz), worldNor);

  const float bias = -0.5f;
  float3 refCenter = worldPos + worldRef * bias;

  float3 up = float3(0.0f, 1.0f, 0.0f);
  if (abs(dot(up, worldRef)) > 0.9f) up.z += 0.1f;
  float3 right = normalize(cross(up, worldRef));
  up = cross(worldRef, right);

  matrix m = {
    right     , dot(right     , -refCenter),
    up        , dot(up        , -refCenter),
    worldRef  , dot(worldRef  , -refCenter),
    0.0f, 0.0f, 0.0f, 1.0f
  };

  const uint refID = tuv.x + tuv.y * RefPntData.z;
  RefViewData[refID].wPos = float4(worldPos, 1.0f);
  RefViewData[refID].wRef = float4(worldRef, 1.0f);
  RefViewData[refID].refMatrix = m;
}