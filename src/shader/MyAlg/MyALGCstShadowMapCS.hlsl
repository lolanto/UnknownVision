// 从场景点中选一个点，放入到各个shadow map中进行比较
StructuredBuffer<Matrix> RefViewMatrixs : register(t0);

Texture2D<float4> scePntPos : register(t1);

RWTexture2D<uint2> shadowMapID : register(u0);
RWTexture2D<float> shadowMapDepth : register(u1);

cbuffer ExtraData : register(b0) {
  // xy: reflect Point size; z: numOfReflect points
  float4 exData;
  // xy: 一块shadow map的大小; zw: shadow map整体大小
  float4 shadowMapSize;
}

struct ScenePoint {
  float4 wPos;
  uint2 id;
};

void paraboloidMapping(inout float4 vert) {
  float len = length(vert.xyz);
  vert.xyz /= len;
  vert.z += 1.0f;
  vert.xy /= vert.z;
  vert.z = len;
  vert.w = 1.0f;
}

const static float near1 = 1.0f;
const static float near2 = 5.0f;

[numthreads(10, 10, 1)]
void main(uint3 GTID : SV_GroupThreadID,
  uint3 GID : SV_GroupID) {
  uint2 tuv = GID.xy * 10 + GTID.xy;

  // 当前点为无效点
  if (scePntPos[tuv].w < 0.00001f) return;

  ScenePoint sp;
  sp.wPos = scePntPos[tuv];
  sp.id = tuv;
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
      float4x4 viewMatrix = RefViewMatrixs[base.x + base.y * smColumn];
      float4 vPos = mul(viewMatrix, sp.wPos);
      // if the point is in the back
      if (vPos.z < 0) continue;
      // paraboloid mapping on vPos
      paraboloidMapping(vPos);
      // calculate vPos's position on shadow map
      vPos.xy = (vPos.xy + 1.0f) / 2.0f;
      vPos.y = 1.0f - vPos.y;
      vPos.x /= smColumn;
      vPos.y /= smRow;
      vPos.x += base.x * smColumnInv;
      vPos.y += base.y * smRowInv;
      // 写入阴影图的位置
      uint2 dPos = vPos.xy * shadowMapSize.zw;
      int level = vPos.z < near1 ? 3 : vPos.z < near2 ? 2 : 1;
      int2 initBias = int2( 1 - level, 1 - level );
      for(;initBias.y < level; ++initBias.y) {
        for(initBias.x = 1 - level; initBias.x < level; ++initBias.x) {
          uint2 curCoord = dPos + initBias;
          // 可能会引入线程间抢占资源导致的错误!!!
          float curDepth = shadowMapDepth[curCoord];
          if (curDepth > vPos.z) {
            shadowMapDepth[curCoord] = vPos.z;
            shadowMapID[curCoord] = sp.id;
          }
        } // for(initBias.x = 1 - level; initBias.x < level; ++initBias.x)
      } // for(;initBias.y < level; ++initBias.y)
    } // for(base.x = 0; base.x < smColumn; ++base.x)
  } // for(; base.y < smRow; ++base.y)
}
