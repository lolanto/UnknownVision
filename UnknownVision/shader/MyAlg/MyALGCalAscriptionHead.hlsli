struct RefEleData {
  float4 wPos;
  float4 wRef;
  float4x4 refMatrix;
};

// 输入像素点属性，判断该像素点应该从哪个反射样本点采样
uint CalculateAscription(
  uint2 refPnt,
  uint numRefPerRow,
  float3 curWorldPos,
  float3 curWorldRef,
  StructuredBuffer<RefEleData> RefData) {

  float minDis = 100000.0f;
  uint minRef = 0;

  for(int2 iter = uint2(-1, -1); iter.y < 2; ++iter.y) {
    for(iter.x = -1; iter.x < 2; ++iter.x) {
      uint2 curPnt = iter + refPnt;
      uint id = curPnt.x + curPnt.y * numRefPerRow;
      float3 refPos = RefData[id].wPos.xyz;
      float3 refDir = RefData[id].wRef.xyz;
      float curDis = 0.2 * length(curWorldPos - refPos)
        // + 0.6 * acos(dot(curWorldRef, refDir));
        + 0.8 * length(curWorldRef - refDir);
      if (curDis < minDis) {
        minRef = id;
        minDis = curDis;
      }
    }
  }

  return minRef;
}