//***************************************************************************************
// TreeSprite.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 mPosH : POSITION;
};

struct VertexOut
{
	float3 mPosH : POSITION;
};

struct GeoOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// 자료를 그대로 기하 셰이더에 넘겨준다.
	vout.mPosH = vin.mPosH;

	return vout;
}

// 각 점을 사각형(정점 4개)으로 확장하므로 기하 셰이더 호출당
// 최대 출력 정점 개수는 4이다.
[maxvertexcount(4)]
void GS(point VertexOut gin[1], // Primitive는 Point이므로 들어오는 정점은 하나이다.
	inout TriangleStream<GeoOut> triStream)
{
	MaterialData matData = gMaterialData[gWidgetMaterialIndex];

	float minPosX = gWidgetAnchorX + (gWidgetPosX * gInvRenderTargetSize.x);
	float minPosY = gWidgetAnchorY + (gWidgetPosY * gInvRenderTargetSize.y);
	float maxPosX = gWidgetAnchorX + ((gWidgetPosX + gWidgetWidth) * gInvRenderTargetSize.x);
	float maxPosY = gWidgetAnchorY + ((gWidgetPosY + gWidgetHeight) * gInvRenderTargetSize.y);

	minPosX = TransformHomogenous(minPosX);
	minPosY = TransformHomogenous(minPosY);
	maxPosX = TransformHomogenous(maxPosX);
	maxPosY = TransformHomogenous(maxPosY);

	float4 v[4];
	v[0] = float4(minPosX, minPosY, 0.0f, 1.0f); // Left Top
	v[1] = float4(minPosX, maxPosY, 0.0f, 1.0f); // Left Bottom
	v[2] = float4(maxPosX, minPosY, 0.0f, 1.0f); // Right Top
	v[3] = float4(maxPosX, maxPosY, 0.0f, 1.0f); // Right Bottom

	float2 texC[4] =
	{
		float2(0.0f, 0.0f), // Left Bottom
		float2(0.0f, 1.0f), // Left Top
		float2(1.0f, 0.0f), // Right Bottom
		float2(1.0f, 1.0f)  // Right Top
	};

	GeoOut gout;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gout.mPosH = v[i];
		gout.mTexC = mul(float4(texC[i], 0.0f, 1.0f), matData.mMatTransform).xy;

		triStream.Append(gout);
	}
}

float4 PS(GeoOut pin) : SV_Target
{
	if (gMaterialIndex == DISABLED)
		return float4(0.0f, 0.0f, 0.0f, 1.0f);

	// 이 픽셀에 사용할 Material Data를 가져온다.
	MaterialData matData = gMaterialData[gWidgetMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;

	// 텍스처 배열의 텍스처를 동적으로 조회한다.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

#ifdef ALPHA_TEST
	// 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
	clip(diffuseAlbedo.a - 0.1f);
#endif

	return diffuseAlbedo;
}