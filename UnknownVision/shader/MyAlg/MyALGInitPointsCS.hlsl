// 初始化场景中的点
Texture2D<float4> wPos : register(t0);
Texture2D<float4> wNormal : register(t1);

RWTexture2D<float4> scePntPos : register(u0);
RWTexture2D<float4> scePntNor : register(u1);
RWTexture2D<uint2> scePntOriUV : register(u2);

cbuffer ExtraData : register(b0) {
  // x: texture size, y: sceSize
  float4 exData;
}

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;
  uint2 base = tuv * (exData.x / exData.y) + (exData.x / exData.y) / 2;
  scePntPos[tuv] = wPos[base];
  scePntNor[tuv] = wNormal[base];
  scePntOriUV[tuv] = base;
}
