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

	// ���� �������� ��ȯ�Ѵ�.
	float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.mPosH = mul(posW, gViewProj);

	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;

	return vout;
}

void PS(VertexOut pin)
{
	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != -1)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
#endif
}