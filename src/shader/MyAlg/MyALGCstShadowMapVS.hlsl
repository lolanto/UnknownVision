Texture2D<float4> ScePntPos : register(t0);
Texture2D<float4> ScePntNor : register(t1);
Texture2D ScePntDiffuse : register(t2);

cbuffer InitPntData : register(b0) {
  // xy: number of scene point per Row and Column
  float4 initPntData;
}

struct VSOutput {
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float4 wDiffuse : TEXCOORD2;
};

VSOutput main(uint VID : SV_VertexID) {
  VSOutput o;
  uint2 tuv = uint2(VID % initPntData.x, VID / initPntData.x);
  o.wPos = ScePntPos[tuv];
  o.wNor = ScePntNor[tuv];
  o.wDiffuse = ScePntDiffuse[tuv];
  return o;
}