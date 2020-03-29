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

	// 세계 공간으로 변환한다.
	float4 posW = mul(float4(vin.posL, 1.0f), gObjWorld);

	// 동차 절단 공간으로 변환한다.
	vout.posH = mul(posW, gViewProj);

	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	vout.texC = mul(float4(vin.texC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

void PS(VertexOut pin)
{
	// 이 픽셀에 사용할 머터리얼 데이터를 가져온다.
	MaterialData materialData = gMaterialData[gObjMaterialIndex];

	float4 diffuse = materialData.diffuseAlbedo;
	uint diffuseMapIndex = materialData.diffuseMapIndex;

	// 텍스처 배열의 텍스처를 동적으로 조회한다.
	if (diffuseMapIndex != DISABLED)
	{
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC);
	}

#ifdef ALPHA_TEST
	clip(diffuse.a - 0.1f);
#endif
}