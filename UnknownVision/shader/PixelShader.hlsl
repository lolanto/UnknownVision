struct v2p {
	float4 position : SV_POSITION;
	float4 t2w0 : TEXCOORD0;
	float4 t2w1 : TEXCOORD1;
	float4 t2w2 : TEXCOORD2;
	float2 texcoord : TEXCOORD3;
};

struct p2m {
	float4 color : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 ORM : SV_TARGET2;
};

SamplerState comSampler : register (s0);
Texture2D baseColor : register (t0);
Texture2D normal : register (t1);
Texture2D ORM : register (t2);

static const float PI = 3.2;

p2m main( v2p i )
{
	p2m o;

	o.color = baseColor.Sample(comSampler, i.texcoord);
	
	o.ORM = ORM.Sample(comSampler, i.texcoord);

	float4 packedNormal = normal.Sample(comSampler, i.texcoord);
	packedNormal.xy = packedNormal.xy * 2 - 1.0;
	packedNormal.xy *= 0.5;
	packedNormal.z = sqrt(1 - saturate(dot(packedNormal.xy, packedNormal.xy)));
	o.normal.x = packedNormal.x * i.t2w0.x + packedNormal.y * i.t2w1.x + packedNormal.z * i.t2w2.x;
	o.normal.y = packedNormal.x * i.t2w0.y + packedNormal.y * i.t2w1.y + packedNormal.z * i.t2w2.y;
	o.normal.z = packedNormal.x * i.t2w0.z + packedNormal.y * i.t2w1.z + packedNormal.z * i.t2w2.z;

	//o.normal = normalize(o.normal);
    o.normal = float4(i.t2w2.xyz, 0.0);

	// 法向量的w分量用来存储线性深度值
	o.normal.w = i.t2w2.w;

	return o;
}