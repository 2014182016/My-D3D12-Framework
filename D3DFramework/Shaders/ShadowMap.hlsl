//***************************************************************************************
// Shadows.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 posL    : POSITION;
	float2 texC    : TEXCOORD;
};

struct VertexOut
{
	float4 posH    : SV_POSITION;
	float2 texC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// ���� �������� ��ȯ�Ѵ�.
	float4 posW = mul(float4(vin.posL, 1.0f), gObjWorld);

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.posH = mul(posW, gViewProj);

	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.texC = mul(float4(vin.texC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

void PS(VertexOut pin)
{
	// �� �ȼ��� ����� ���͸��� �����͸� �����´�.
	MaterialData materialData = gMaterialData[gObjMaterialIndex];

	float4 diffuse = materialData.diffuseAlbedo;
	uint diffuseMapIndex = materialData.diffuseMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC);
	}

#ifdef ALPHA_TEST
	clip(diffuse.a - 0.1f);
#endif
}