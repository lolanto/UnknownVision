Texture2D<float4> SSWPos : register(t0);
Texture2D<float4> SSWNor : register(t1);
Texture2D<uint> SSID : register(t2);
Texture2D<uint> SSIndex : register(t3);


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
  // const uint2 bound = uint2(RefPntData.zw);
  // const uint2 refSubSize = uint2(RefPntData.xy / RefPntData.zw);
  // const uint2 halfRefSubSize = refSubSize / 2;
  // uint2 tuv = uint2(VID % RefPntData.x, VID / RefPntData.x);

  // float3 curWPos = SSWPos[tuv].xyz;
  // float3 curWNor = normalize(SSWNor[tuv].xyz);

  // float minDis = 1000000.0f;
  // uint2 minIter = uint2(0, 0);

  // for(uint2 iter = uint2(0, 0); iter.y < bound.y; ++iter.y) {
  //   for(iter.x = 0; iter.x < bound.x; ++iter.x) {
  //     uint2 tarTexPos = iter * refSubSize + halfRefSubSize;
  //     float3 tarWPos = SSWPos[tarTexPos].xyz;
  //     float3 tarWNor = normalize(SSWNor[tarTexPos].xyz);
  //     float factor = -dot(tarWNor, curWNor);
  //     float dis = 0.1f * length(tarWPos - curWPos)
  //       + 0.9f * 5.0f * (factor + 1.0f);
  //     if (minDis > dis) {
  //       minDis = dis;
  //       minIter = iter;
  //     }
  //   }
  // }
  // minIter.y = 9 - minIter.y;
  // VSOutput o;
  // o.WPos = curWPos;
  // o.WNor = curWNor;
  // o.pos = float4(0.2f * minIter -0.9f, 0.5f, 1.0f);

  int2 ScreenSize;
  SSID.GetDimensions(ScreenSize.x, ScreenSize.y);
  uint2 tuv = uint2(VID % ScreenSize.x, VID / ScreenSize.x);


  float3 curWPos = SSWPos[tuv].xyz;
  float3 curWNor = normalize(SSWNor[tuv].xyz);  

  VSOutput o;
  o.WPos = curWPos;
  o.WNor = curWNor;

  uint faceID = SSID[tuv];
  if (faceID == 0xffffffffu) {
    o.pos = float4(0, 0, -1.5f, 1.0f); // discard this point!
    return o;
  }
  // 当前所在的块
  uint2 tile = tuv / (ScreenSize / uint2(5, 5));
  uint2 slot = uint2((tile.x + tile.y * 5) * 36 + faceID, 0);

  uint ascription = SSIndex[slot];
  // ascription = 23;
  uint2 minIter = uint2(ascription % RefPntData.z + 10e-4, ascription / RefPntData.z);
  minIter.y = RefPntData.w - 1 - minIter.y;
  const float2 interval = 1.0f / RefPntData.zw;
  const float2 b = interval - 1.0f;
  const float2 a = (2.0f - interval * 2.0f) / (RefPntData.zw - 1.0f);
  // 与refPntData.zw相关
  o.pos = float4(a * minIter + b, 0.5f, 1.0f);

  return o;
}
