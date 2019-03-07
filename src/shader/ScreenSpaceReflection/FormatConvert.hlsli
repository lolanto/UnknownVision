#ifndef FORMAT_CONVERT
#define FORMAT_CONVERT

uint FLOAT_TO_UINT(float _V, float _Scale) {
  return (uint)floor(_V * _Scale + 0.5f);
}

float Saturate_FLOAT(float _V) {
  return min(max(_V, 0), 1);
}

float4 R8G8B8A8_UNORM_to_FLOAT4(uint packedInput) {
  float4 unpackedOutput;
  unpackedOutput.x = (float)  (   packedInput         & 0x000000ff)  / 255;
  unpackedOutput.y = (float)  ((( packedInput >> 8 )  & 0x000000ff)) / 255;
  unpackedOutput.z = (float)  ((( packedInput >> 16)  & 0x000000ff)) / 255;
  unpackedOutput.w = (float)  (   packedInput >> 24)                 / 255;
  return unpackedOutput;
}

uint FLOAT4_to_R8G8B8A8_UNORM(float4 unpackedInput) {
  uint packedOutput;
  packedOutput = ((FLOAT_TO_UINT(Saturate_FLOAT(unpackedInput.x), 255))       |
                  (FLOAT_TO_UINT(Saturate_FLOAT(unpackedInput.y), 255) << 8)  |
                  (FLOAT_TO_UINT(Saturate_FLOAT(unpackedInput.z), 255) << 16) |
                  (FLOAT_TO_UINT(Saturate_FLOAT(unpackedInput.w), 255) << 24) );
  return packedOutput;
}

#endif