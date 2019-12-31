#include "Common.hlsl"

struct VertexIn
{
	float3 mPosH : POSITION;
	float2 mTexC : TEXCOORD;
};

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	MaterialData matData = gMaterialData[gWidgetMaterialIndex];

	vout.mPosH = float4(vin.mPosH, 1.0f);
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	if (gWidgetMaterialIndex == DISABLED)
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
