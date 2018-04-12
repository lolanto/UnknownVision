#ifndef PS_INPUT
#define PS_INPUT

cbuffer CameraData : register(b0) {
	matrix GViewMatrix;
  matrix GViewMatrixInv;
	matrix GProjectMatrix;
	matrix GProjectMatrixInv;
	float4 GEyePos;
  // x: n, y: f, z: width, w: height
	float4 GCameraParam;
  /* 
  xy: nearPlaneSize.xy
  zw: farPlaneSize.xy
  */
  float4 GCameraParam2;
}

#endif
