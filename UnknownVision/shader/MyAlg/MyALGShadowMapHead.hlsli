void paraboloidMapping(inout float4 vert) {
  float len = length(vert.xyz);
  vert.xyz /= len;
  vert.z += 1.0f;
  vert.xy /= vert.z;
  vert.z = len;
  vert.w = 1.0f;
}

static const float2 OrthViewSize = float2(4.0f, 2.0f);
static const float2 OrthViewHalfSize = OrthViewSize / 2.0f;
static const float2 OrthViewNearFar = float2(0.1f, 10.0f);
static const float OrthViewFactorA =
  (OrthViewNearFar.x * OrthViewNearFar.y) / (OrthViewNearFar.x - OrthViewNearFar.y);
static const float OrthViewFactorB = 
  (OrthViewNearFar.y) / (OrthViewNearFar.y - OrthViewNearFar.x);

void orthogonalMapping(inout float4 vert) {
  vert.xy /= OrthViewHalfSize;
  if (abs(vert.x) > 1 ||
    abs(vert.y) > 1) {
    vert.w = -1;
  } else {
    vert.w = 1;
  }
  vert.z = OrthViewFactorA * (1.0f / vert.z) + OrthViewFactorB;
}

void fromNDCtoViewSpace(inout float4 vert) {
  // flip y axis
  vert.y = -vert.y;
  // from -1~1 to 0~1
  vert.xy = (vert.xy + 1.0f) / 2.0f;
}