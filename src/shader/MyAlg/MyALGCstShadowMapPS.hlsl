struct GSOutput {
  float4 sPos : SV_POSITION;
  float4 wPos : TEXCOORD0;
  float4 wNor : TEXCOORD1;
  float4 wDiffuse : TEXCOORD2;
};

struct PSOutput {
  float4 wPos : SV_Target0;
  float4 wNor : SV_Target1;
  float4 wDiffuse : SV_Target2;
};

PSOutput main(GSOutput i) {
  PSOutput o;
  o.wPos = i.wPos;
  o.wNor = i.wNor;
  o.wDiffuse = i.wDiffuse;
  return o;
}
