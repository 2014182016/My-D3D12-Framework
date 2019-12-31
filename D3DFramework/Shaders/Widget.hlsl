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
