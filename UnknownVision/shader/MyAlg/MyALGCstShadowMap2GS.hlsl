struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4 vRef;
  float4 wNor;
  float4x4 refMatrix;
  float4x4 refProjMatrix;
};

StructuredBuffer<RefEleData> RefViewMatrixs : register(t0);

cbuffer ProjectMatrixData : register(b0) {
  // x: fov, y: aspect, z: near, w: far
  float4 PerProjSettingData;
  float4x4 PerProjMatrix;
  float4x4 PerProjMatrixInv;
  // x: width, y: height, z: near, w: far
  float4 OrtProjSettingData;
  float4x4 OrtProjMatrix;
  float4x4 OrtProjMatrixInv;
}

cbuffer RefPntData : register(b1) {
  // xy: Screen Width And Height; 
  //zw: Number of Reflection Sample point per Row and Column
  float4 RefPntData;
}

cbuffer InstanceData : register(b2) {
  uint4 InstanceData;
}

cbuffer CameraData : register(b3) {
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

struct VSInput {
  float4 pos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
};

struct GSOutput {
  float4 pos : SV_POSITION;
  uint viewPortIndex : SV_ViewportArrayIndex;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float4 tNor : TEXCOORD2;
  float4 tPos : TEXCOORD3;
};

[maxvertexcount(30)]
void main(
  in triangle VSInput input[3],
  inout TriangleStream<GSOutput> output) {

  uint2 iter = uint2(0, InstanceData.x);
  const uint2 bound = uint2(RefPntData.zw);
  for(; iter.x < bound.x; ++iter.x) {
    uint refIndex = iter.x + iter.y * bound.x;
    float4x4 viewMatrix = RefViewMatrixs[refIndex].refMatrix;
    // float4x4 ProjMatrix = RefViewMatrixs[refIndex].refProjMatrix;
    float3 vRef = RefViewMatrixs[refIndex].vRef.xyz;
    float4 tNor = RefViewMatrixs[refIndex].wNor;
    float4 tPos = RefViewMatrixs[refIndex].wPos;
    vRef = vRef / -vRef.z;
    for(uint i = 0; i < 3; ++i) {
      GSOutput o;
      o.pos = input[i].pos;
      o.wPos = input[i].wPos;
      o.wNor = input[i].wNor;
      o.pos = mul(viewMatrix, o.pos);
      // o.pos.xy += vRef.xy * o.pos.z;
      o.pos = mul(PerProjMatrix, o.pos);
      // o.pos = mul(OrtProjMatrix, o.pos);
      // o.pos = mul(GProjectMatrix, o.pos);
      o.viewPortIndex = iter.x;
      o.tNor = tNor;
      o.tPos = tPos;
      output.Append(o);
    }
    output.RestartStrip();
  }
}