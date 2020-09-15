Texture2D<float4> Input : register(t0);

RWTexture2D<float4> Output : register(u0);

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  int2 tuv = GID.xy * 10 + GTID.xy;

/*
1 1  1
1 -8 1
1 1  1
*/
  float4 ref = float4(0, 0, 0, 0);
  ref += Input[tuv + int2(-1, -1)];
  ref += Input[tuv + int2( 0, -1)];
  ref += Input[tuv + int2( 1, -1)];
  ref += Input[tuv + int2(-1,  0)];
  ref += Input[tuv + int2( 1,  0)];
  ref += Input[tuv + int2(-1,  1)];
  ref += Input[tuv + int2( 0,  1)];
  ref += Input[tuv + int2( 1,  1)];
  ref -= Input[tuv] * 8;

  if (dot(ref, ref) > 0.01f) Output[tuv] = float4(0, 0, 0, 0);
  else Output[tuv] = float4(1, 1, 1, 1);
}