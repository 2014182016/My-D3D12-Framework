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

// ���� �� ǥ���� World Space�� ��ȯ�Ѵ�.
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	// �� ������ [0,1]���� [-1,1]�� ����Ѵ�.
	float3 normalT = 2.0f*normalMapSample - 1.0f;

	// �������� ������ �����Ѵ�.
	float3 n = unitNormalW;
	float3 t = normalize(tangentW - dot(tangentW, n)*n);
	float3 b = cross(n, t);

	float3x3 tbn = float3x3(t, b, n);

	// Tangent Space���� World Space�� ��ȯ�Ѵ�.
	float3 bumpedNormalW = mul(normalT, tbn);

	return bumpedNormalW;
}

float3 BoxCubeMapLookup(float3 rayOrigin, float3 unitRayDir, float3 boxCenter, float3 boxExtents)
{
	// ������ �߽��� �������� �д�.
	float3 p = rayOrigin - boxCenter;

	// AABB�� i��° ���� ������ �� ��� ���� ������ ������ ����.
	// t1 = (-dot(n_i), p) + h_i) / dot(n_i, d) = (-p_i + h_i) / d_i;
	// t2 = (-dot(n_i), p) - h_i) / dot(n_i, d) = (-p_i - h_i) / d_i;

	// ����ȭ�� �� ��� ��鿡 ���� ������ �� ��� ������ �Ѳ����� �����Ѵ�.
	float3 t1 = (-p + boxExtents) / unitRayDir;
	float3 t2 = (-p - boxExtents) / unitRayDir;

	// �� ��ǥ������ �ִ밪�� ã�´�.
	// �������� ���� ���ο� �ִٰ� �����ϹǷ�, ���� �Ű������� �ִ밪�� ã���� �ȴ�.
	float3 tmax = max(t1, t2);

	// tmax�� ��� ������ �ּڰ��� ���Ѵ�.
	float t = min(min(tmax.x, tmax.y), tmax.z);

	// ť�� ���� ��ȸ ���ͷ� ����� �� �ֵ��� ������ �߽ɿ� ������� ��ǥ�� �����.
	return p + t * unitRayDir;
}

float CalcShadowFactor(float4 shadowPosH, int shadowMapIndex)
{
	// w�� �������ν� ������ �Ϸ��Ѵ�.
	shadowPosH.xyz /= shadowPosH.w;

	// �׸��� ���� ������ ��� ���� �������� �׸��ڸ� �帮���� �ʴ´�.
	if (shadowPosH.x < 0.0f || shadowPosH.x > 1.0f || shadowPosH.y < 0.0f || shadowPosH.y > 1.0f)
		return 1.0f;

	// NDC ���������� ����
	float depth = shadowPosH.z;

	uint width, height, numMips;
	gShadowMaps[shadowMapIndex].GetDimensions(0, width, height, numMips);

	// �ؼ� ������
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
		// 4ǥ�� PCF�� �ڵ����� ����ȴ�.
		percentLit += gShadowMaps[shadowMapIndex].SampleCmpLevelZero(gsamShadow,
			shadowPosH.xy + offsets[i], depth).r;
	}

	// 0�� �־��� ���� ������ �׸��� �ȿ� �ִٴ� ���̰�,
	// 1�� �׸��ڸ� ������ ����ٴ� ���̴�.
	return percentLit / 9.0f;
}

float4 ComputeShadowLighting(StructuredBuffer<Light> lights, Material mat, float3 pos, float3 normal, float3 toEye)
{
	float3 result = 0.0f;

	for (uint i = 0; i < LIGHT_NUM; ++i)
	{
		if (lights[i].enabled == 0)
			continue;

		// �ٸ� ��ü�� ������ ������ ���� shadowFactor�� �ȼ��� ��Ӱ� �Ѵ�.
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
�Ȱ� �Ӽ��� ���� ������ ���´�.
*/
float4 FogBlend(float4 litColor, float distToEye)
{
	float4 result = 0.0f;

	switch (gFogType)
	{
	case FOG_LINEAR:
	{
		// �Ÿ��� ���� �Ȱ� ������ ���� ����� ����Ѵ�.
		float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
		result = lerp(litColor, gFogColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL:
	{
		// ���������� �ָ� �������� �Ȱ��� �� �β�������.
		float fogAmount = exp(-distToEye * gFogDensity);
		result = lerp(gFogColor, litColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL2:
	{
		// �ſ� �β��� �Ȱ��� ��Ÿ����.
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