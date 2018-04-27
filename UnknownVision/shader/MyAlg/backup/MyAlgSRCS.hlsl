Texture2D GSMUV : register (t0);
Texture2D SSPos : register (t1);
Texture2D SSRef : register (t2);
Texture2D SMAlbedo : register (t3);
StructuredBuffer<Matrix> catBuf : register (t4);

RWTexture2D<unorm float4> SRAlbedo : register (u0);

[numthreads(10, 10, 1)]
void main (uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 uv = GID * 10 + GTID;
  // 标准化后的uv坐标(0.0 ~ 1.0)
  float2 nrmUV;
  nrmUV.x = uv.x / 1280.0f;
  nrmUV.y = uv.y / 960.0f;
  // 将范围扩大到[0, 10)
  uint2 intUV;
  intUV.x = floor(nrmUV.x * 10);
  intUV.y = floor(nrmUV.y * 10);
  // 将反射向量转移到目标区域中
  float3 refDir = SSRef.Load(uint3(uv, 0)).xyz;
  refDir = mul(catBuf[intUV.y * 10 + intUV.x], float4(refDir, 0.0f)).xyz;
  refDir = normalize(refDir);

  //计算采样的UV(-1.0 ~ 1.0)
  refDir.z += 1.0f;
  refDir.xyz = normalize(refDir.xyz);
  float2 samUV = refDir.xy / refDir.z;
  samUV.xy += 1.0f;
  samUV.y = 2.0f - samUV.y;
  samUV.xy /= 20.0f;

  // 计算绝对采样坐标(0 ~ 1280/960)
  samUV += intUV * 0.1f;
  intUV = samUV * 1280;

  uint2 smuv = GSMUV.Load(uint3(intUV, 0)).xy * 2048;
  float4 temp = SMAlbedo.Load(uint3(smuv, 0));
  SRAlbedo[uv] = temp;
}