cbuffer camBuf : register (b0) {
  matrix leftViewMat;
  matrix rightViewMat;
  matrix topViewMat;
  matrix bottomViewMat;
  matrix forwardViewMat;
  matrix backwardViewMat;
  matrix projMatrix;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
  float2 texcoord : TEXCOORD0;
  uint slice : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(
	triangle float4 input[3] : SV_POSITION, 
  triangle float2 uv[3] : TEXCOORD0,
	inout TriangleStream< GSOutput > output){
  // for (uint j = 0; j < 6; j++) {
  //   for (uint i = 0; i < 3; i++)
	 //  {
		//   GSOutput element;
		//   element.pos = mul(projMatrix, mul(leftViewMat, input[i]));
  //     // element.pos = mul(projMatrix, input[i]);
  //     element.texcoord = uv[i];
  //     element.slice = j;
	 //   	output.Append(element);
	 //  }
  // }
  uint i = 0;
  for (; i < 3; ++i) {
    GSOutput ele;
    ele.pos = mul(projMatrix, mul(leftViewMat, input[i]));
    ele.texcoord = uv[i];
    ele.slice = 0;
    output.Append(ele);
  }
  output.RestartStrip();
  i = 0;
  for (; i < 3; ++i) {
    GSOutput ele;
    ele.pos = mul(projMatrix, mul(rightViewMat, input[i]));
    ele.texcoord = uv[i];
    ele.slice = 1;
    output.Append(ele);
  }
  output.RestartStrip();
  i = 0;
  for (; i < 3; ++i) {
    GSOutput ele;
    ele.pos = mul(projMatrix, mul(topViewMat, input[i]));
    ele.texcoord = uv[i];
    ele.slice = 2;
    output.Append(ele);
  }
  output.RestartStrip();
  i = 0;
  for (; i < 3; ++i) {
    GSOutput ele;
    ele.pos = mul(projMatrix, mul(bottomViewMat, input[i]));
    ele.texcoord = uv[i];
    ele.slice = 3;
    output.Append(ele);
  }
  output.RestartStrip();
  i = 0;
  for (; i < 3; ++i) {
    GSOutput ele;
    ele.pos = mul(projMatrix, input[i]);
    ele.texcoord = uv[i];
    ele.slice = 4;
    output.Append(ele);
  }
  output.RestartStrip();
  i = 0;
  for (; i < 3; ++i) {
    GSOutput ele;
    ele.pos = mul(projMatrix, mul(backwardViewMat, input[i]));
    ele.texcoord = uv[i];
    ele.slice = 5;
    output.Append(ele);
  }
}