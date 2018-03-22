Texture2D SSWorldPos : register (t0);
Texture2D SSWorldNor : register (t1);

RWStructuredBuffer<Matrix> catBuf : register (u0);

[numthreads(10, 10, 1)]
void main (uint3 index : SV_GroupThreadID) {
  uint3 pos = uint3(index.x * 128 + 64, index.y * 96 + 48, 0);
  float3 worldPos = SSWorldPos.Load(pos).xyz;
  float3 worldNor = SSWorldNor.Load(pos).xyz;
  worldNor = normalize(worldNor);

  // 采样点向前偏移
  worldPos += worldNor * 0.01f;
  float3 up = float3(0.0f, 1.0f, 0.0f);
  if (abs(dot(up, worldNor)) > 0.9f) up.z += 0.1f;
  float3 right = normalize(cross(up, worldNor));
  up = cross(worldNor, right);

  Matrix m = {
    right   , dot(right   , -worldPos), // row 1
    up      , dot(up      , -worldPos), // row 2
    worldNor, dot(worldNor, -worldPos), // row 3
    0.0f, 0.0f, 0.0f, 1.0f              // row 4
  };

  catBuf[index.y * 10 + index.x] = m;
}
