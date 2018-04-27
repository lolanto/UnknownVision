struct GSInput {
  float4 pos : SV_POSITION;
  uint viewPortIndex : SV_ViewportArrayIndex;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
};

struct PSOutput {
  float4 wPos : SV_Target0;
  float4 wNor : SV_Target1;
};

PSOutput main(GSInput i) {
  PSOutput o;
  o.wPos = i.wPos;
  o.wNor = i.wNor;
  return o;
}