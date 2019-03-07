#include "./MyALGShadowMapHead.hlsli"

// 从场景点中选一个点，放入到各个shadow map中进行比较
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

RWTexture2D<uint2> ShadowMapTexPos : register(u0);
RWTexture2D<float> ShadowMapDepth : register(u1);

cbuffer ExtraData : register(b0) {
  // xy: reflect Point size; z: numOfReflect points
  float4 exData;
  // xy: 一块shadow map的大小; zw: shadow map整体大小
  float4 shadowMapSize;
}

struct ScenePoint {
  float4 wPos;
  uint2 texPos; // 场景图中的Texel位置
};

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;

  // 当前点为无效点
  if (ScePntPos[tuv].w < 0.00001f) return;

  ScenePoint sp;
  sp.wPos = ScePntPos[tuv];
  sp.texPos = tuv;
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
      // 此处会产生资源访问冲突!
      float curDepth = ShadowMapDepth[texPos];
      if (curDepth > vPos.z) {
        ShadowMapDepth[texPos] = vPos.z;
        ShadowMapTexPos[texPos] = sp.texPos;
      }
    } // for(base.x = 0; base.x < smColumn; ++base.x)
  } // for(; base.y < smRow; ++base.y)
}