#include "./MyALGShadowMapHead.hlsli"
// 展示当前场景中选取的反射点在当前反射策略下所见的内容
struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4 vRef;
  float4 wNor;
  float4x4 refMatrix;
  float4x4 refProjMatrix;
};

StructuredBuffer<RefEleData> RefViewMatrixs : register(t0);

Texture2D<float4> ScePntPos : register(t1);
Texture2D<float4> BasicColor : register(t2);

RWTexture2D<unorm float4> shadowMapDiffuse : register(u0);
RWTexture2D<uint> shadowMapDepth : register(u1);

cbuffer ExtraData : register(b0) {
  // xy: reflect Point size; z: numOfReflect points
  float4 exData;
  // xy: 一块shadow map的大小; zw: shadow map整体大小
  float4 shadowMapSize;
}

cbuffer GeoData : register(b1) {
  // xy: Number of Scene Point per Row and Column;
  float4 GeoData;
}

struct ScenePoint {
  float4 wPos;
  float4 diffuse;
};

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;

  // 当前点为无效点
  if (ScePntPos[tuv].w < 0.00001f) return;

  ScenePoint sp;
  sp.wPos = ScePntPos[tuv];
  float2 uv = float2(tuv) / GeoData.xy;
  uint2 textureSize;
  BasicColor.GetDimensions(textureSize.x, textureSize.y);
  sp.diffuse = BasicColor[uint2(uv * textureSize)];
  // 反射采样点的总量
  const uint maxRefPnt = exData.z;
  // 阴影图中每一行，每一列有多少个小阴影图
  const uint smRow = shadowMapSize.w / shadowMapSize.y;
  const uint smColumn = shadowMapSize.z / shadowMapSize.x;
  const float smRowInv = 1.0f / smRow;
  const float smColumnInv = 1.0f / smColumn;
  uint2 base = uint2(0, 0);
  
  for(; base.y < smRow; ++base.y) {
    for(base.x = 0; base.x < smColumn; ++base.x) {
      float4x4 viewMatrix = RefViewMatrixs[base.x + base.y * smColumn].refMatrix;
      float4 vPos = mul(viewMatrix, sp.wPos);
      // if the point is in the back
      if (vPos.z < 0) continue;
      // Orth projection on vPos
      orthogonalMapping(vPos);
      if (vPos.w < 0.000001f) continue;
      // calculate vPos's position on shadow map
      fromNDCtoViewSpace(vPos);
      vPos.x /= smColumn;
      vPos.y /= smRow;
      vPos.x += base.x * smColumnInv;
      vPos.y += base.y * smRowInv;
      // calculate the texel position
      uint2 texPos = vPos.xy * shadowMapSize.zw;
      uint vDepth = vPos.z * 10000.0f;
      uint oriDepth;
      // // 此处会产生资源访问冲突!
      // float curDepth = shadowMapDepth[texPos];
      // if (curDepth > vPos.z) {
      //   shadowMapDepth[texPos] = vPos.z;
      //   shadowMapDiffuse[texPos] = sp.diffuse;
      // }
      InterlockedMin(shadowMapDepth[texPos], vDepth, oriDepth);
      if (vDepth + 100 < oriDepth) {
        shadowMapDiffuse[texPos] = sp.diffuse;
      }
    } // for(base.x = 0; base.x < smColumn; ++base.x)
  } // for(; base.y < smRow; ++base.y)
}