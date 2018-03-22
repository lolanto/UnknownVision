struct GSOutput {
  float4 tPos : SV_POSITION;
  float4 position : TEXCOORD0;
  float2 texcoord : TEXCOORD1;
  uint slide : SV_RenderTargetArrayIndex;
};

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 texcoord : TEXCOORD0;
};

[maxvertexcount(6)]
void main(
  in triangle VSOutput input[3],
  inout TriangleStream< GSOutput > output) {
  for (uint j = 0; j < 2; ++j) {
    for (uint i = 0; i < 3; ++i) {
      GSOutput ele;
      ele.tPos = ele.position = input[i].pos;
      ele.texcoord = input[i].texcoord;
      ele.slide = j;
      output.Append(ele);
    }
    output.RestartStrip();
  }
}