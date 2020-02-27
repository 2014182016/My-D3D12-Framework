#include "Common.hlsl"

static float4 topColor = float4(0.0f, 0.05f, 0.6f, 1.0f);
static float4 centerColor = float4(0.7f, 0.7f, 0.92f, 1.0f);
static float skyScale = 5000.0f;

struct VertexIn
{
	float3 mPosL : POSITION;
	float2 mTexC : TEXCOORD;
};

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float3 mPosW : POSITION;
	float2 mTexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// World Space�� ��ȯ�Ѵ�.
	float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);
	vout.mPosW = posW.xyz;

	// z = w�̸� �ش� �ȼ��� �׻� far plane�� ��ġ���ִ�.
	vout.mPosH = mul(posW, gViewProj).xyww;

	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

[earlydepthstencil]
float4 PS(VertexOut pin) : SV_Target
{
	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	float diffuseIntensity = 1.0f;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseIntensity = gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC).r;
	}

	float height = pin.mPosW.y / skyScale;
	if (height < 0.0f)
	{
		height *= -1.0f;
	}
	float4 skyColor = lerp(topColor, centerColor, height);
	float4 outColor = skyColor + diffuseIntensity;

	return outColor;
}

