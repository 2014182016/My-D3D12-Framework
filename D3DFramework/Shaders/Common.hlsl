//***************************************************************************************
// Common.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "LightingUtil.hlsl"

#define DISABLED -1

#define TEX_NUM 8
#define CUBE_MAP_NUM 1

struct MaterialData
{
	float4x4 mMatTransform;
	float4   mDiffuseAlbedo;
	float3   mSpecular;
	float    mRoughness;
	uint     mDiffuseMapIndex;
	uint     mNormalMapIndex;
	uint     mMatPad1;
	uint     mMatPad2;
};

Texture2D gTextureMaps[TEX_NUM] : register(t0, space0);
Texture2D gShadowMap[LIGHT_NUM]: register(t0, space1);
TextureCube gCubeMaps[CUBE_MAP_NUM] : register(t0, space2);

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
    float4x4 gWorld;
	uint gMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
	float4x4 gIdentity;
    float3 gEyePosW;
	uint gCurrentSkyCubeMapIndex;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	bool gFogEnabled;
	float gPadding0;
};

cbuffer cbWidget :register(b2)
{
	uint gWidgetMaterialIndex;
	uint gWidgetPosX;
	uint gWidgetPosY;
	uint gWidgetWidth;

	uint gWidgetHeight;
	float gWidgetAnchorX;
	float gWidgetAnchorY;
	uint gWidgetPadding0;
}

cbuffer cbDebug : register(b3)
{
	float4x4 gDebugWorld;
	float4 gDebugColor;
}

// [0, 1] 범위의 값을 [-1, 1]로 변환한다.
float TransformHomogenous(float pos)
{
	return (pos * 2.0f) - 1.0f;
}

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

	// NDC 공간에서의 깊이
	float depth = shadowPosH.z;

	uint width, height, numMips;
	gShadowMap[shadowMapIndex].GetDimensions(0, width, height, numMips);

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
		percentLit += gShadowMap[shadowMapIndex].SampleCmpLevelZero(gsamShadow,
			shadowPosH.xy + offsets[i], depth).r;
	}

	// 0은 주어진 점이 완전히 그림자 안에 있다는 뜻이고,
	// 1은 그림자를 완전히 벗어났다는 뜻이다.
	return percentLit / 9.0f;
}

