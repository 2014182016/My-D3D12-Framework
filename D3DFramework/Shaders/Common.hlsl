//***************************************************************************************
// Common.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtil.hlsl"

struct MaterialData
{
	float4   mDiffuseAlbedo;
	float3   mFresnelR0;
	float    mRoughness;
	float4x4 mMatTransform;
	uint     mDiffuseMapIndex;
	uint     mNormalMapIndex;
	uint     mMatPad1;
	uint     mMatPad2;
};

// ���̴� ���� 5.1 �̻� �����ϴ� �ؽ�ó �迭 Texture2DArray�ʹ� 
// �޸� �� �迭���� ũ��� ������ �ٸ� �ؽ�ó���� ���� �� �ִ�. 
Texture2D gTextureMaps[7] : register(t0);

// space1�� ������ MaterialData�� space0�� ��ġ�� �ؽ��� �迭�� ��ġ�� �ʴ´�.
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
	float4x4 gTexTransform;
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
    float3 gEyePosW;
    float cbPerObjectPad1;
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
	float2 cbPerObjectPad2;

    // [0, NUM_DIR_LIGHTS) - Directional Light
    // [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) - Point Light
    // [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS) - Spot Light
    Light gLights[MaxLights];
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