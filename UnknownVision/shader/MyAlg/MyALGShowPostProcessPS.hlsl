Texture2D basicColor : register(t0);
Texture2D mapData : register(t1);
SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

bool SimpleEdgeTest(uint2 cTexPos, Texture2D target) {
  /*
  1 1  1
  1 -8 1
  1 1  1
  */
  uint2 base[8];
  base[0] = cTexPos + uint2(0, -1);
  base[1] = cTexPos + uint2(-1, 0);
  base[2] = cTexPos + uint2(1, 0);
  base[3] = cTexPos + uint2(0, 1);
  base[4] = cTexPos + uint2(-1, -1);
  base[5] = cTexPos + uint2(1, -1);
  base[6] = cTexPos + uint2(-1, 1);
  base[7] = cTexPos + uint2(1, 1);

  float4 result = -8.0 * target[cTexPos]
    + target[base[0]]
    + target[base[1]]
    + target[base[2]]
    + target[base[3]]
    + target[base[4]]
    + target[base[5]]
    + target[base[6]]
    + target[base[7]];
  return dot(result, result) > 0.01f;
}

float4 main(VSOutput i) : SV_Target {
  // uint2 texSize;
  // mapData.GetDimensions(texSize.x, texSize.y);
  // if (SimpleEdgeTest(uint2(texSize * i.uv), mapData))
  //   return float4(0, 0, 0, 1);
  // else return mapData.Sample(linearSampler, i.uv);
  // return mapData.Sample(linearSampler, i.uv);
  return basicColor.Sample(linearSampler, mapData.Sample(pointSampler, i.uv).xy);
}