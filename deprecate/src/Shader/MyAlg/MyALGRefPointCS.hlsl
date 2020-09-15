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

[numthreads(10, 10, 1)]
void main(uint3 GTID: SV_GroupThreadID,
  uint3 GID: SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;
  uint2 base = tuv * (exData.xy / exData.zw) + (exData.xy / exData.zw) / 2;
  
  float3 worldPos = SSWPos[base].xyz;
  float3 worldNor = normalize(SSWNor[base].xyz);

  float3 up = float3(0.0f, 1.0f, 0.0f);
  if (abs(dot(up, worldNor)) > 0.9f) up.z += 0.1f;
  float3 right = normalize(cross(up, worldNor));
  up = cross(worldNor, right);

  Matrix m = {
    right   , dot(right   , -worldPos), // row1
    up      , dot(up      , -worldPos), // row2
    worldNor, dot(worldNor, -worldPos), // row3
    0.0f, 0.0f, 0.0f, 1.0f              // row4
  };

  RefViewMatrixs[tuv.x + tuv.y * exData.z].wPos = float4(worldPos, 1.0f);
  RefViewMatrixs[tuv.x + tuv.y * exData.z].wRef = float4(worldNor, 0.0f);
  RefViewMatrixs[tuv.x + tuv.y * exData.z].refMatrix = m;
}
