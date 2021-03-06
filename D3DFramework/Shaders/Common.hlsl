//***************************************************************************************
// Common.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "LightingUtil.hlsl"
#include "Pass.hlsl"

Texture2D gTextureMaps[TEX_NUM] : register(t0, space0);
Texture2D gShadowMaps[LIGHT_NUM]: register(t0, space1);

Texture2D gDiffuseMap : register(t0, space2);
Texture2D gSpecularRoughnessMap : register(t1, space2);
Texture2D gPositonMap : register(t2, space2);
Texture2D gNormalMap : register(t3, space2);
Texture2D gNormalMapx : register(t4, space2); // Normal Map x
Texture2D gDepthMap : register(t5, space2);
Texture2D gSsaoMap : register(t6, space2);
Texture2D gSsrMap : register(t7, space2);
Texture2D gBluredSsrMap : register(t8, space2);

StructuredBuffer<Light> gLights : register(t0, space3);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space3);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

cbuffer cbPerObject : register(b0)
{
    float4x4 gObjWorld;
	uint gObjMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};

// 법선 맵 표본을 World Space로 변환한다.
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	// 각 성분을 [0,1]에서 [-1,1]로 사상한다.
	float3 normalT = 2.0f*normalMapSample - 1.0f;

	// 정규직교 기저를 구축한다.
	float3 n = unitNormalW;
	float3 t = normalize(tangentW - dot(tangentW, n)*n);
	float3 b = cross(n, t);

	float3x3 tbn = float3x3(t, b, n);

	// Tangent Space에서 World Space로 변환한다.
	float3 bumpedNormalW = mul(normalT, tbn);

	return bumpedNormalW;
}

float3 BoxCubeMapLookup(float3 rayOrigin, float3 unitRayDir, float3 boxCenter, float3 boxExtents)
{
	// 상자의 중심을 원점으로 둔다.
	float3 p = rayOrigin - boxCenter;

	// AABB의 i번째 평판 반직선 대 평면 교차 공식은 다음과 같다.
	// t1 = (-dot(n_i), p) + h_i) / dot(n_i, d) = (-p_i + h_i) / d_i;
	// t2 = (-dot(n_i), p) - h_i) / dot(n_i, d) = (-p_i - h_i) / d_i;

	// 벡터화한 후 모든 평면에 대해 반직선 대 평면 공식을 한꺼번에 적용한다.
	float3 t1 = (-p + boxExtents) / unitRayDir;
	float3 t2 = (-p - boxExtents) / unitRayDir;

	// 각 좌표성분의 최대값을 찾는다.
	// 반직선이 상자 내부에 있다고 가정하므로, 교차 매개변수의 최대값만 찾으면 된다.
	float3 tmax = max(t1, t2);

	// tmax의 모든 성분의 최솟값을 구한다.
	float t = min(min(tmax.x, tmax.y), tmax.z);

	// 큐브 맵의 조회 벡터로 사용할 수 있도록 상자의 중심에 상대적인 좌표로 만든다.
	return p + t * unitRayDir;
}

float CalcShadowFactor(float4 shadowPosH, int shadowMapIndex)
{
	// w를 나눔으로써 투영을 완료한다.
	shadowPosH.xyz /= shadowPosH.w;

	// 그림자 맵의 범위를 벗어난 것은 라이팅을 그림자를 드리우지 않는다.
	if (shadowPosH.x < 0.0f || shadowPosH.x > 1.0f || shadowPosH.y < 0.0f || shadowPosH.y > 1.0f)
		return 1.0f;

	// NDC 공간에서의 깊이
	float depth = shadowPosH.z;

	uint width, height, numMips;
	gShadowMaps[shadowMapIndex].GetDimensions(0, width, height, numMips);

	// 텍셀 사이즈
	float dx = 1.0f / (float)width;

	float percentLit = 0.0f;
	const float2 offsets[9] =
	{
		float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
	};

	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		// 4표본 PCF가 자동으로 수행된다.
		percentLit += gShadowMaps[shadowMapIndex].SampleCmpLevelZero(gsamShadow,
			shadowPosH.xy + offsets[i], depth).r;
	}

	// 0은 주어진 점이 완전히 그림자 안에 있다는 뜻이고,
	// 1은 그림자를 완전히 벗어났다는 뜻이다.
	return percentLit / 9.0f;
}

float4 ComputeShadowLighting(StructuredBuffer<Light> lights, Material mat, float3 pos, float3 normal, float3 toEye)
{
	float3 result = 0.0f;

	for (uint i = 0; i < LIGHT_NUM; ++i)
	{
		if (lights[i].enabled == 0)
			continue;

		// 다른 물체에 가려진 정도에 따라 shadowFactor로 픽셀을 어둡게 한다.
		float4 shadowPosH = mul(float4(pos, 1.0f), gLights[i].shadowTransform);
		float3 shadowFactor = CalcShadowFactor(shadowPosH, i);

		switch (lights[i].type)
		{
		case DIRECTIONAL_LIGHT:
			result += shadowFactor * ComputeDirectionalLight(lights[i], mat, normal, toEye);
			break;
		case POINT_LIGHT:
			result += ComputePointLight(lights[i], mat, pos, normal, toEye);
			break;
		case SPOT_LIGHT:
			result += shadowFactor * ComputeSpotLight(lights[i], mat, pos, normal, toEye);
			break;
		}
	}

	return float4(result, 0.0f);
}

/*
안개 속성에 따라 색상을 섞는다.
*/
float4 FogBlend(float4 litColor, float distToEye)
{
	float4 result = 0.0f;

	switch (gFogType)
	{
	case FOG_LINEAR:
	{
		// 거리에 따라 안개 색상을 선형 감쇠로 계산한다.
		float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
		result = lerp(litColor, gFogColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL:
	{
		// 지수적으로 멀리 있을수록 안개가 더 두꺼워진다.
		float fogAmount = exp(-distToEye * gFogDensity);
		result = lerp(gFogColor, litColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL2:
	{
		// 매우 두꺼운 안개를 나타낸다.
		float fogAmount = exp(-pow(distToEye * gFogDensity, 2));
		result = lerp(gFogColor, litColor, fogAmount);
		break;
	}
	}

	return result;
}

float4x4 GetMaterialTransform(uint materialIndex)
{
	if (materialIndex == DISABLED)
		return gIdentity;
	return gMaterialData[materialIndex].matTransform;
}