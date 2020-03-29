#include "Common.hlsl"

struct VertexIn
{
	float3 posH : POSITION;
	float2 texC : TEXCOORD;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float2 texC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.posH = float4(vin.posH, 1.0f);
	vout.texC = mul(float4(vin.texC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	if (gObjMaterialIndex == DISABLED)
		return float4(0.0f, 0.0f, 0.0f, 1.0f);

	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];

	float4 diffuseAlbedo = matData.diffuseAlbedo;
	uint diffuseMapIndex = matData.diffuseMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamLinearWrap, pin.texC);
	}

	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuseAlbedo.a - 0.1f);

	return diffuseAlbedo;
}
