//***************************************************************************************
// Shadows.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 mPosL    : POSITION;
	float2 mTexC    : TEXCOORD;
};

struct VertexOut
{
	float4 mPosH    : SV_POSITION;
	float2 mTexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	MaterialData matData = gMaterialData[gObjMaterialIndex];

	// 세계 공간으로 변환한다.
	float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);

	// 동차 절단 공간으로 변환한다.
	vout.mPosH = mul(posW, gViewProj);

	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;

	return vout;
}

void PS(VertexOut pin)
{
	// 이 픽셀에 사용할 Material Data를 가져온다.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;

	// 텍스처 배열의 텍스처를 동적으로 조회한다.
	if (diffuseMapIndex != -1)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
#endif
}