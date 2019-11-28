//***************************************************************************************
// Common.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "LightingUtil.hlsl"

struct MaterialData
{
	float4   mDiffuseAlbedo;
	float3   mSpecular;
	float    mRoughness;
	float4x4 mMatTransform;
	uint     mDiffuseMapIndex;
	uint     mNormalMapIndex;
	uint     mMatPad1;
	uint     mMatPad2;
};

struct DebugData
{
	float4x4 gBoundingWorld;
};

// 셰이더 모형 5.1 이상만 지원하는 텍스처 배열 Texture2DArray와는 
// 달리 이 배열에는 크기와 형식이 다른 텍스처들을 담을 수 있다. 
Texture2D gTextureMaps[7] : register(t0);

// space1에 배정한 MaterialData는 space0에 위치한 텍스터 배열과 겹치지 않는다.
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);
StructuredBuffer<DebugData> gDebugData : register(t1, space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

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
    float gPadding0;
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
	float gPadding1;

	Light gLights;
};

cbuffer cbInstance : register(b2)
{
	uint gInstanceIndex;
	uint gDebugInstanceIndex;
	uint gInstPad1;
	uint gInstPad2;
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