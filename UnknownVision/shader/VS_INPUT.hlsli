// Ԥ����������ʽ�Լ��ض��ṩ��ȫ�ֱ���

struct a2v {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 texcoord : TEXCOORD;
};

cbuffer TransformMatrix : register(b1) {
	matrix GModelMatrix;
	matrix GModelMatrixInv;
}

cbuffer ViewMatrix : register(b2) {
	matrix GViewMatrix;
	matrix GProjectMatrix;
	matrix GProjectMatrixInv;
	float4 GEyePos;
	float4 GCameraParam; // x: n, y: f, z: n*f, w: n-f
}
