#include "PBRFuncs.hlsli"
#include "SSRFuncs.hlsli"

struct v2p {
	float4 position : SV_POSITION;
	float4 eye : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
};

cbuffer lightBuffer : register (b1) {
	float4 numLight;
	struct {
		float4 color;
		float4 direction;
		float4 position;
	} directLight[2];

	struct {
		float4 colorNradius;
		float4 position;
	} pointLight[2];
}

SamplerState pointSampler : register (s0);
SamplerState comSampler : register (s1);
Texture2D baseColor : register (t0);
Texture2D normal : register (t1);
Texture2D ORM : register (t2);
Texture2D<float> DepthMap : register(t3);
Texture2D<float> BackFaceDepthMap : register(t4);

float4 main(v2p i) : SV_TARGET {

	float4 norAndLinearZ = normal.Sample(pointSampler, i.texcoord);
	float3 csNor = normalize(norAndLinearZ.xyz);
	float3 csOrig = reconstCSPos2(i.texcoord, norAndLinearZ.w);
	float3 csDir = normalize(reflect(csOrig, csNor));
	float rayLength = ((csOrig.z + csDir.z * maxDistance) < GCameraParam.x) ?
	 (GCameraParam.x - csOrig.z) / csDir.z : maxDistance;

	float3 csEndPoint = csOrig + csDir * rayLength;
	float4 H = mul(GProjectMatrix, float4(csEndPoint, 1.0f));
	float k0 = 1 / norAndLinearZ.w;
	float k1 = 1 / H.w;

	float2 p0 = float2(i.texcoord);
	float2 p1 = float2(H.xy * k1 / 2 + 0.5f);
	p1.y = 1.0f - p1.y;

	p1 += (distanceSquared(p0, p1) < 0.0000001f) ? float2(0.0001f, 0.0001f) : 0.0f;

	float2 delta = p1 - p0;

	bool permute = false;
	if (abs(delta.x) < abs(delta.y)) {
		permute = true;
		delta = delta.yx;
		p0 = p0.yx;
		p1 = p1.yx;
	}

	float stepDir = sign(delta.x);
	float invdx = stepDir / delta.x;

	// clip to normalize coordinate
	clipCoordinate(p0, p1);

	float dk = (k1 - k0) * invdx;
	float2 dp = float2(stepDir, delta.y * invdx);

	float3 pk = float3(p0, k0);
	float3 dpk = float3(dp, dk);

	float end = p1.x * stepDir;

	float stepCount = 0.0f;

	float rayZ = csOrig.z;
	float sceneZ = csOrig.z + GCameraParam.y;
	float2 curStride;

	/* Depth Mipmap */	
	// float curLevel = 0;
	// float maxLevel = 10;
	// bool rollBack = false;
	// for (;
	// 	((pk.x * stepDir) < end) &&
	// 	(stepCount < maxSteps) &&
	// 	(curLevel < maxLevel) && (curLevel >= 0); ++stepCount) {
	// 	if (!rollBack) {
	// 		normal.GetDimensions(curLevel, curStride.x, curStride.y, maxLevel);
	// 		curStride = permute ? curStride.yx : curStride.xy;
	// 		curStride = 1 / curStride;

	// 		pk += dpk * curStride.x;
	// 		rayZ = 1 / pk.z;
	// 	}
	// 	sceneZ = normal.SampleLevel(pointSampler, permute ? pk.yx : pk.xy, curLevel).w;
	// 	if (rayZ < sceneZ) {
	// 		if (rollBack) rollBack = false;
	// 		else ++curLevel;
	// 	}
	// 	else {
	// 		--curLevel;
	// 		rollBack = true;
	// 	}
	// }
	// if (curLevel < 0 && rayZ - sceneZ < 0.07f) return baseColor.Sample(pointSampler, permute ? pk.yx : pk.xy);
	
	/* Traditional Method */

	// normal.GetDimensions(curLevel, curStride.x, curStride.y, maxLevel);
	// curStride = permute ? curStride.yx : curStride.xy;
	// curStride = 1 / curStride;
	// for (;
	// 	((pk.x * stepDir) < end) &&
	// 	(stepCount < maxSteps); ++stepCount) {

	// 	pk += dpk * curStride.x * 5;
	// 	rayZ = 1 / pk.z;

	// 	sceneZ = normal.SampleLevel(pointSampler, permute ? pk.yx : pk.xy, curLevel).w;
	// 	if (rayZ > sceneZ) return baseColor.Sample(pointSampler, permute ? pk.yx : pk.xy);
	// }

	/* Traditional Method with Binary Search */

	float strideScale = 16.0f;
	bool apprx = false;
	float threshold = 0.0f;

	normal.GetDimensions(curStride.x, curStride.y);
	curStride = permute ? curStride.yx : curStride.xy;
	curStride = 1 / curStride;

	pk += dpk * curStride.x;

	for (;
		((pk.x * stepDir) < end) &&
		(stepCount < maxSteps); ++stepCount) {

		rayZ = 1 / pk.z;

		sceneZ = normal.SampleLevel(pointSampler, permute ? pk.yx : pk.xy, 0).w;
		if (rayZ < sceneZ) {
			if (apprx) strideScale = abs(strideScale) * 0.5f;
			// else strideScale = abs(strideScale) * 2;
		} else {
			float thickness = BackFaceDepthMap.Sample(pointSampler, permute ? pk.yx : pk.xy).r - sceneZ;
			if (thickness < 0) thickness = 1.#INF;
			if (rayZ - sceneZ < 0.1f) return baseColor.Sample(pointSampler, permute ? pk.yx : pk.xy);
			else if (rayZ < sceneZ + thickness) {
				strideScale = -0.5f * abs(strideScale);
				apprx = true;
			}
			else {
				if (apprx) strideScale = abs(strideScale) * 0.5f;
				// else strideScale = abs(strideScale) * 2;
			}
			// else {
			// 	strideScale = -0.5f * abs(strideScale);
			// 	apprx = true;
			// }
		}

		pk += dpk * curStride.x * strideScale;
		// with edge fake!
		if (pk.x * stepDir > end) {
			float t = (p1.x - pk.x) / dpk.x;
			pk += dpk * t;
			strideScale += t / curStride.x;
			pk -= dpk * curStride.x;
			rayZ = 1 / pk.z;
			sceneZ = normal.SampleLevel(pointSampler, permute ? pk.yx : pk.xy, 0).w;
			if (rayZ < sceneZ) break;
			apprx = true;
		}
	}
	return float4(0, 0, 0, 1);
}

// float4 main(v2p i) : SV_TARGET{
// 	if (baseColor.Sample(pointSampler, i.texcoord).w < 0.0001) discard;
// 	float4 NorAndLinearZ = normal.Sample(pointSampler, i.texcoord);
// 	// float3 csNor = normalize(normal.Sample(pointSampler, i.texcoord).xyz);
// 	float3 csNor = normalize(NorAndLinearZ.xyz);

// 	// float3 csOrig = reconstCSPos(i.texcoord, DepthMap);
// 	float3 csOrig = reconstCSPos2(i.texcoord, NorAndLinearZ.w);
// 	float3 csDir = normalize(reflect(csOrig, csNor));
// 	float2 temp = float2(0, 0);
// 	//temp = traceScreenSpaceRay(csOrig, csDir, DepthMap, temp);
// 	//return float4(temp / GCameraParam.zw, 0.0, 1);
// 	// return float4(0, traceScreenSpaceRay(csOrig, csDir, normal, temp), 0, 1);
// 	if (traceScreenSpaceRay(csOrig, csDir, normal, temp))
// 		return baseColor.Load(int3(temp, 0));
// 	else
// 		return float4(0, 0, 0, 1);
// 	return baseColor.Load(int3(i.position.xy, 0));
// 	// return float4(0, NorAndLinearZ.w / GCameraParam.y, 0, 1);
// }

//float4 main( v2p i ) : SV_TARGET
//{
//    float4 base = baseColor.Sample(pointSampler, i.texcoord);
//    if (base.w < 0.001)
//        discard;
//    // gamma decode
//    base.xyz = GammaDecode(base.xyz);
//	float4 orm = ORM.Sample(pointSampler, i.texcoord);
//	float3 n = normal.Sample(pointSampler, i.texcoord).xyz;
//	float4 p = worldPosition.Sample(pointSampler, i.texcoord);
//
//    float4 color = float4(0.0, 0.0, 0.0, 0.0);
//
//    float3 viewDir = normalize(i.eye.xyz - p.xyz);
//	float3 mF0 = lerp(F0, base.xyz, orm.z);
//    float kDL = (orm.g + 1) * (orm.g + 1) / 8;
//
//    float NdotV = max(dot(n, viewDir), 0.0);
//    float3 Ks = Fschlick(NdotV, mF0);
//    float3 Kd = 1 - Ks;
//
//	// iterate direction light
//	for (int count = 0; count < numLight.x; ++count) {
//		float3 radiance = directLight[count].color.xyz;
//        float3 H = normalize(directLight[count].direction.xyz + viewDir);
//        
//        float NdotL = max(dot(n, directLight[count].direction.xyz), 0.0);
//        float NdotH = max(dot(n, H), 0.0);
//        color.xyz += (PBRBRDF(NdotV, NdotL, NdotH, orm.xyz, Ks, kDL) + Kd * base.xyz / PI) * radiance * NdotL;
//    }
//	// iterate point light
//	for (count = 0; count < numLight.y; ++count) {
//		float3 lightDir = (pointLight[count].position - p).xyz;
//		float distance = length(lightDir);
//		if (distance > pointLight[count].colorNradius.w) continue;
//		lightDir = normalize(lightDir);
//        float3 radiance = pointLight[count].colorNradius.xyz / (1 + 0.14 * distance + 0.2 * distance * distance);
//        float3 H = normalize(viewDir + lightDir).xyz;
//        
//        float NdotL = max(dot(n, lightDir), 0.0);
//        float NdotH = max(dot(n, H), 0.0);
//        color.xyz += (PBRBRDF(NdotV, NdotL, NdotH, orm.xyz, Ks, kDL) + Kd * base.xyz / PI) * radiance * NdotL;
//    }
//    color.xyz += 0.001 * orm.r;
//    color.xyz = ToonMapping(color.xyz);
//    color.xyz = GammaEncode(color.xyz);
//    return color;
//}