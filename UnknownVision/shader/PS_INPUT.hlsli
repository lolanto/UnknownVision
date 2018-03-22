#ifndef PS_INPUT
#define PS_INPUT

cbuffer CameraData : register(b0) {
	matrix GViewMatrix;
	matrix GProjectMatrix;
	matrix GProjectMatrixInv;
	float4 GEyePos;
	float4 GCameraParam; // x: n, y: f, z: width, w: height
}

#endif
