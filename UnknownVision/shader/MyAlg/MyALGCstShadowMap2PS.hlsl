struct GSInput {
  float4 pos : SV_POSITION;
  uint viewPortIndex : SV_ViewportArrayIndex;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float4 tNor : TEXCOORD2;
  float4 tPos : TEXCOORD3;
  float4 wTan : TEXCOORD4;
  float4 wBin : TEXCOORD5;
  float4 uvDuv : TEXCOORD6;
};

struct PSOutput {
  float4 wPos : SV_Target0;
  float4 wNor : SV_Target1;
  float4 wTan : SV_Target2;
  float4 wBin : SV_Target3;
  float4 uvDuv : SV_Target4;
};

PSOutput main(GSInput i) {
  if (dot(i.wPos.xyz - i.tPos.xyz, i.tNor.xyz) < 0) discard;
  PSOutput o;
  o.wPos = i.wPos;
  o.wNor = i.wNor;
  o.wTan = i.wTan;
  o.wBin = i.wBin;
  o.uvDuv = i.uvDuv;
  return o;
}