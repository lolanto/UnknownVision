struct VSOutput {
  float4 pos : SV_POSITION;
  float3 WPos : TEXCOORD0;
  float3 WNor : TEXCOORD1;
};

struct PSOutput {
  float4 WPos : SV_Target0;
  float4 WNor : SV_Target1;
};

PSOutput main(VSOutput i) {
  PSOutput o;
  o.WPos = float4(i.WPos, 1.0f);
  o.WNor = float4(i.WNor, 1.0f);
  return o;
}