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

	// �ڷḦ �״�� ���� ���̴��� �Ѱ��ش�.
	vout.mPosH = vin.mPosH;

	return vout;
}

// �� ���� �簢��(���� 4��)���� Ȯ���ϹǷ� ���� ���̴� ȣ���
// �ִ� ��� ���� ������ 4�̴�.
[maxvertexcount(4)]
void GS(point VertexOut gin[1], // Primitive�� Point�̹Ƿ� ������ ������ �ϳ��̴�.
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

	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gWidgetMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

#ifdef ALPHA_TEST
	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuseAlbedo.a - 0.1f);
#endif

	return diffuseAlbedo;
}