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

// [0, 1] ������ ���� [-1, 1]�� ��ȯ�Ѵ�.
float TransformHomogenous(float pos)
{
	return (pos * 2.0f) - 1.0f;
}

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

	// NDC ���������� ����
	float depth = shadowPosH.z;

	uint width, height, numMips;
	gShadowMap[shadowMapIndex].GetDimensions(0, width, height, numMips);

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
		percentLit += gShadowMap[shadowMapIndex].SampleCmpLevelZero(gsamShadow,
			shadowPosH.xy + offsets[i], depth).r;
	}

	// 0�� �־��� ���� ������ �׸��� �ȿ� �ִٴ� ���̰�,
	// 1�� �׸��ڸ� ������ ����ٴ� ���̴�.
	return percentLit / 9.0f;
}

