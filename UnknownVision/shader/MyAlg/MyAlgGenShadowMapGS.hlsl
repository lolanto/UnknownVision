
struct VSOutput {
  float4 pos: SV_POSITION;
  float2 uv : TEXCOORD0;
};

struct GSOutput {
  float4 pos: SV_POSITION;
  float2 uv : TEXCOORD0;
};

StructuredBuffer<Matrix> MatBuf : register (t0);

void projVert(inout float4 vert, int2 jk);

[maxvertexcount(150)]
[instance(10)]
void main(
  in triangle VSOutput input[3],
  uint InstanceID : SV_GSInstanceID,
  inout TriangleStream<GSOutput> output) {
  uint2 jk;
  for (jk.y = InstanceID * 1; jk.y < InstanceID * 1 + 1; ++jk.y) {
    for (jk.x = 0; jk.x < 10; ++jk.x) {
      GSOutput ele[3];
      float3 eleState = float3(0, 0, 0);
      ele[0].pos = mul(MatBuf[jk.y * 10 + jk.x], input[0].pos);
      if (ele[0].pos.z < 0) eleState.x = 1;
      ele[0].uv = input[0].uv;

      ele[1].pos = mul(MatBuf[jk.y * 10 + jk.x], input[1].pos);
      if (ele[1].pos.z < 0) eleState.y = 1;
      ele[1].uv = input[1].uv;

      ele[2].pos = mul(MatBuf[jk.y * 10 + jk.x], input[2].pos);
      if (ele[2].pos.z < 0) eleState.z = 1;
      ele[2].uv = input[2].uv;

      float res = dot(eleState, float3(1, 1, 1));

      if (res > 2) continue;
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
        float2 Duv = ele[mpIndex].uv - ele[lpIndex].uv;
        if (Dpos.z == 0) Dpos.z += 0.01f;
        float k = - ele[lpIndex].pos.z / Dpos.z;
        eleL.pos.xy = ele[lpIndex].pos.xy + Dpos.xy * k;
        eleL.pos.z = 0;
        eleL.pos.w = 1;
        eleL.uv = ele[lpIndex].uv + Duv * k;

        Dpos = ele[mpIndex].pos - ele[rpIndex].pos;
        Duv = ele[mpIndex].uv - ele[rpIndex].uv;
        if (Dpos.z == 0) Dpos.z += 0.01f;
        k = -ele[rpIndex].pos.z / Dpos.z;
        eleR.pos.xy = ele[rpIndex].pos.xy + Dpos.xy * k;
        eleR.pos.z = 0;
        eleR.pos.w = 1;
        eleR.uv = ele[rpIndex].uv + Duv * k;
        if (res > 1) {
          projVert(eleL.pos, jk);
          projVert(ele[mpIndex].pos, jk);
          projVert(eleR.pos, jk);
          output.Append(eleL);
          output.Append(ele[mpIndex]);
          output.Append(eleR);
          output.RestartStrip();
        }
        else {
          projVert(ele[lpIndex].pos, jk);
          projVert(eleL.pos, jk);
          projVert(eleR.pos, jk);
          projVert(ele[rpIndex].pos, jk);
  
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
        // 都在正面
        projVert(ele[0].pos, jk);
        projVert(ele[1].pos, jk);
        projVert(ele[2].pos, jk);
        output.Append(ele[0]);
        output.Append(ele[1]);
        output.Append(ele[2]);
        output.RestartStrip();
      }
    }
  }
}

void projVert(inout float4 vert, int2 jk) {
  float len = length(vert.xyz);
  vert.xyz /= len;
  vert.z += 1.0f;
  vert.xyz = normalize(vert.xyz);
  vert.xy /= vert.z;
  vert.z = (len - 1.0f) / 960.0f;
  vert.w = 1.0f;

  vert.xy /= 10.0f;
  vert.x -= 0.9f;
  vert.x += jk.x * 0.2f;
  vert.y += 0.9f;
  vert.y -= jk.y * 0.2f;
}
