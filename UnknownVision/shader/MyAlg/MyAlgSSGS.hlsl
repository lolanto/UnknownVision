struct VSOutput {
  float4 pos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 worldNor : TEXCOORD1;
  float4 worldRef : TEXCOORD2;
};

struct GSOutput {
  float4 pos : SV_POSITION;
  float4 worldPos : TEXCOORD0;
  float4 worldNor : TEXCOORD1;
  float4 worldRef : TEXCOORD2;
};

void projVert(inout float4 vert) {
  float len = length(vert.xyz);
  vert.xyz /= len;
  vert.z += 1.0f;
  vert.xy /= vert.z;
  vert.z = (len - 1.0f) / 960.0f;
  vert.w = 1.0f;
}

[maxvertexcount(6)]
void main(
  in triangle VSOutput input[3],
  inout TriangleStream<GSOutput> output) {
  GSOutput ele[3];
  float3 eleState = float3(0, 0, 0);
  ele[0].pos = input[0].pos;
  if (ele[0].pos.z < 0) eleState.x = 1;
  ele[0].worldPos = input[0].worldPos;
  ele[0].worldNor = input[0].worldNor;
  ele[0].worldRef = input[0].worldRef;

  ele[1].pos = input[1].pos;
  if (ele[1].pos.z < 0) eleState.y = 1;
  ele[1].worldPos = input[1].worldPos;
  ele[1].worldNor = input[1].worldNor;
  ele[1].worldRef = input[1].worldRef;

  ele[2].pos = input[2].pos;
  if (ele[2].pos.z < 0) eleState.z = 1;
  ele[2].worldPos = input[2].worldPos;
  ele[2].worldNor = input[2].worldNor;
  ele[2].worldRef = input[2].worldRef;

  float res = dot(eleState, float3(1, 1, 1));
  if (res > 2) return;
  else if (res != 0) {
    int mpIndex = 0, lpIndex = 1, rpIndex = 2;
    if (res > 1) {
      // 两个点在背面
      if (eleState.y == 0) {
        mpIndex = 1;
        lpIndex = 0;
        rpIndex = 2;
      }
      else if (eleState.z == 0) {
        mpIndex = 2;
        lpIndex = 1;
        rpIndex = 0;
      }
    }
    else {
      // 一个点在背面
      mpIndex = dot(eleState, float3(0, 1, 2));
      lpIndex = mpIndex - 1 < 0 ? 2 : mpIndex - 1;
      rpIndex = mpIndex + 1 > 2 ? 0 : mpIndex + 1;
    }

    GSOutput eleL,eleR;
    float3 Dpos = ele[mpIndex].pos - ele[lpIndex].pos;
    float4 DworldPos = ele[mpIndex].worldPos - ele[lpIndex].worldPos;
    float4 DworldNor = ele[mpIndex].worldNor - ele[lpIndex].worldNor;
    float4 DworldRef = ele[mpIndex].worldRef - ele[lpIndex].worldRef;
    if (Dpos.z == 0) Dpos.z += 0.01f;
    float k = - ele[lpIndex].pos.z / Dpos.z;
    eleL.pos.xy = ele[lpIndex].pos.xy + Dpos.xy * k;
    eleL.pos.z = 0;
    eleL.pos.w = 1;
    eleL.worldPos = ele[lpIndex].worldPos + DworldPos * k;
    eleL.worldNor = ele[lpIndex].worldNor + DworldNor * k;
    eleL.worldRef = ele[lpIndex].worldRef + DworldRef * k;

    Dpos = ele[mpIndex].pos - ele[rpIndex].pos;
    DworldPos = ele[mpIndex].worldPos - ele[rpIndex].worldPos;
    DworldNor = ele[mpIndex].worldNor - ele[rpIndex].worldNor;
    DworldRef = ele[mpIndex].worldRef - ele[rpIndex].worldRef;
    if (Dpos.z == 0) Dpos.z += 0.01f;
    k = -ele[rpIndex].pos.z / Dpos.z;
    eleR.pos.xy = ele[rpIndex].pos.xy + Dpos.xy * k;
    eleR.pos.z = 0;
    eleR.pos.w = 1;
    eleR.worldPos = ele[lpIndex].worldPos + DworldPos * k;
    eleR.worldNor = ele[lpIndex].worldNor + DworldNor * k;
    eleR.worldRef = ele[lpIndex].worldRef + DworldRef * k;

    if (res > 1) {
      projVert(eleL.pos);
      projVert(ele[mpIndex].pos);
      projVert(eleR.pos);
      output.Append(eleL);
      output.Append(ele[mpIndex]);
      output.Append(eleR);
      output.RestartStrip();
    }
    else {
      projVert(ele[lpIndex].pos);
      projVert(eleL.pos);
      projVert(eleR.pos);
      projVert(ele[rpIndex].pos);
  
      output.Append(ele[lpIndex]);
      output.Append(eleL);
      output.Append(eleR);
      output.RestartStrip();
  
      output.Append(ele[lpIndex]);
      output.Append(eleR);
      output.Append(ele[rpIndex]);
      output.RestartStrip();
    }
  }
  else {
    projVert(ele[0].pos);
    projVert(ele[1].pos);
    projVert(ele[2].pos);
    output.Append(ele[0]);
    output.Append(ele[1]);
    output.Append(ele[2]);
    output.RestartStrip();
  }
}
