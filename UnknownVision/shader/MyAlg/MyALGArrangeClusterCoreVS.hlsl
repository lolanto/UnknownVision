Texture2D<float4> SSWPos : register(t0);
Texture2D<float4> SSWNor : register(t1);


cbuffer RefPntData : register(b0) {
  // xy: Screen Width And Height; 
  //zw: Number of Reflection Sample point per Row and Column
  float4 RefPntData;
}

struct VSOutput {
  float4 pos : SV_POSITION;
  float3 WPos : TEXCOORD0;
  float3 WNor : TEXCOORD1;
};

VSOutput main(uint VID : SV_VertexID) {
  const uint2 bound = uint2(RefPntData.zw);
  const uint2 refSubSize = uint2(RefPntData.xy / RefPntData.zw);
  const uint2 halfRefSubSize = refSubSize / 2;
  uint2 tuv = uint2(VID % RefPntData.x, VID / RefPntData.x);

  float3 curWPos = SSWPos[tuv].xyz;
  float3 curWNor = SSWNor[tuv].xyz;

  float minDis = 1000000.0f;
  uint2 minIter = uint2(0, 0);

  for(uint2 iter = uint2(0, 0); iter.y < bound.y; ++iter.y) {
    for(iter.x = 0; iter.x < bound.x; ++iter.x) {
      uint2 tarTexPos = iter * refSubSize + halfRefSubSize;
      float3 tarWPos = SSWPos[tarTexPos].xyz;
      float3 tarWNor = SSWNor[tarTexPos].xyz;
      float dis = 0.7 * length(tarWPos - curWPos)
        + length(tarWNor - curWNor);
      if (minDis > dis) {
        minDis = dis;
        minIter = iter;
      }
    }
  }
  minIter.y = 9 - minIter.y;
  VSOutput o;
  o.WPos = curWPos;
  o.WNor = curWNor;
  o.pos = float4(0.2f * minIter -0.9f, 0.5f, 1.0f);

  return o;
}
