// 展示当前场景的点化情况
Texture2D<float4> ScePntPos : register(t0);
Texture2D<float4> BasicColor : register(t1);

RWTexture2D<float> Depth : register(u0);
RWTexture2D<unorm float4> Res : register(u1);

cbuffer CameraData : register(b0) {
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

cbuffer GeoData : register(b1) {
  // x: texture size, y: sceSize
  float4 GeoData;
}

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;
  float4 curPnt = ScePntPos[tuv];
  // 当前为无效点
  if (curPnt.w < 0.000001f) return;

  curPnt = 
    mul(GProjectMatrix,
      mul(GViewMatrix, curPnt));
  // 裁剪空间剔除
  if (curPnt.w < GCameraParam.x) return;
  if (curPnt.w > GCameraParam.y) return;
  if (abs(curPnt.x) > curPnt.w) return;
  if (abs(curPnt.y) > curPnt.w) return;
  // 透视除法——NDC
  curPnt /= curPnt.w;

  // flip y axis
  curPnt.y = -curPnt.y;
  // from -1~1 to 0~1
  curPnt.xy = (curPnt.xy + 1.0f) / 2.0f;
  uint2 texPos = curPnt.xy * GCameraParam.zw;
  float curDepth = Depth[texPos];
  if (curDepth > curPnt.z) {
    Depth[texPos] = curPnt.z;
    float2 uv = float2(tuv) / GeoData.xy;
    uint2 textureSize;
    BasicColor.GetDimensions(textureSize.x, textureSize.y);
    Res[texPos] = BasicColor[uint2(uv * textureSize)];
  }
}
