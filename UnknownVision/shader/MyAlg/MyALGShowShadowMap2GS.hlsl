struct RefEleData{
  float4 wPos;
  float4 wRef;
  float4x4 refMatrix;
};

StructuredBuffer<RefEleData> RefViewMatrixs : register(t0);

cbuffer ProjectMatrixData : register(b0) {
  // 投影矩阵
  float4x4 proMatrix;
}

cbuffer RefPntData : register(b1) {
  // xy: Screen Width And Height; 
  //zw: Number of Reflection Sample point per Row and Column
  float4 RefPntData;
}

cbuffer InstanceData : register(b2) {
  uint4 InstanceData;
}

struct VSInput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

struct GSOutput {
  float4 pos : SV_POSITION;
  uint viewPortIndex : SV_ViewportArrayIndex;
  float2 uv : TEXCOORD0;
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
    
    for(uint i = 0; i < 3; ++i) {
      GSOutput o;
      o.pos = input[i].pos;
      o.uv = input[i].uv;
      o.pos = mul(proMatrix, mul(viewMatrix, o.pos));
      o.viewPortIndex = iter.x;
      output.Append(o);
    }
    output.RestartStrip();
  }
}