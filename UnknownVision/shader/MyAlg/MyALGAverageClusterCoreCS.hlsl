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
  float4 vRef;
  float4 wNor;
  float4x4 refMatrix;
  float4x4 refProjMatrix;
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

  // const float bias = -0.0f;
  // float3 refCenter = worldPos + worldRef * bias;
  // refCenter.y = refCenter.y < 2.8f ? 2.8f : refCenter.y;
  float3 refCenter = GEyePos.xyz - 2 * dot((GEyePos.xyz - worldPos), worldNor) * worldNor;
  float3 refDir = normalize(worldPos - refCenter);

  float3 up = float3(0.0f, 1.0f, 0.0f);
  if (abs(dot(up, refDir)) > 0.9f) up.z += 0.1f;
  float3 right = normalize(cross(up, refDir));
  up = cross(refDir, right);

  matrix m = {
    right     , dot(right     , -refCenter),
    up        , dot(up        , -refCenter),
    refDir  , dot(refDir  , -refCenter),
    0.0f, 0.0f, 0.0f, 1.0f
  };

  // float3 up = float3(0.0f, 1.0f, 0.0f);
  // if (abs(dot(up, worldNor)) > 0.9f) up.z += 0.1f;
  // float3 right = normalize(cross(up, worldNor));
  // up = cross(worldNor, right);

  // matrix m = {
  //   right     , dot(right     , -refCenter),
  //   up        , dot(up        , -refCenter),
  //   worldNor  , dot(worldNor  , -refCenter),
  //   0.0f, 0.0f, 0.0f, 1.0f
  // };

  // float3 up = float3(0.0f, 1.0f, 0.0f);
  // if (abs(dot(up, worldRef)) > 0.9f) up.z += 0.1f;
  // float3 right = normalize(cross(up, worldRef));
  // up = cross(worldRef, right);

  // matrix m = {
  //   right     , dot(right     , -refCenter),
  //   up        , dot(up        , -refCenter),
  //   worldRef  , dot(worldRef  , -refCenter),
  //   0.0f, 0.0f, 0.0f, 1.0f
  // };

  float4 viewRef = mul(m, float4(worldRef, 0.0f));

  // fov = 90deg 1.57rad
  // aspect = 1.0f
  // far = 100.0f
  float2 nf;
  // nf.x = 100.0f / (100.0f - viewRef.z);
  // nf.y = (viewRef.z * 100.0f) / (viewRef.z - 100.0f);
  nf.x = 1.001;
  nf.y = -0.1001;
  matrix p = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, nf,
    0, 0, 1, 0
  };

  const uint refID = tuv.x + tuv.y * RefPntData.z;
  RefViewData[refID].wPos = float4(worldPos, 1.0f);
  RefViewData[refID].wRef = float4(worldRef, 1.0f);
  RefViewData[refID].vRef = viewRef;
  RefViewData[refID].wNor = float4(worldNor, 1.0f);
  RefViewData[refID].refMatrix = m;
  RefViewData[refID].refProjMatrix = p;
}