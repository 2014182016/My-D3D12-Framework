#include "Common.hlsl"

struct VertexIn
{
	float3 mPosL     : POSITION;
	float3 mNormalL  : NORMAL;
	float3 mTangentU : TANGENT;
	float3 mBinormalU: BINORMAL;
	float2 mTexC     : TEXCOORD;
};

struct VertexOut
{
	float4 mPosH      : SV_POSITION;
	float3 mPosW      : POSITION0;
	float3 mNormalW   : NORMAL;
	float3 mTangentW  : TANGENT;
	float3 mBinormalW : BINORMAL;
	float2 mTexC      : TEXCOORD;
};

struct PixelOut
{
	float4 mDiffuse  : SV_TARGET0;
	float4 mSpecularAndRoughness : SV_TARGET1;
	float4 mNormal   : SV_TARGET2;
	float4 mPosition : SV_TARGET3;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// �� ������ ����� Material�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];

	// World Space�� ��ȯ�Ѵ�.
	float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);
	vout.mPosW = posW.xyz;

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.mPosH = mul(posW, gViewProj);

	// World Matrix�� ��յ� ��ʰ� ���ٰ� �����ϰ� Normal�� ��ȯ�Ѵ�.
	// ��յ� ��ʰ� �ִٸ� ����ġ ����� ����ؾ� �Ѵ�.
	vout.mNormalW = mul(vin.mNormalL, (float3x3)gObjWorld);
	vout.mTangentW = mul(vin.mTangentU, (float3x3)gObjWorld);
	vout.mBinormalW = mul(vin.mBinormalU, (float3x3)gObjWorld);

	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;

	return vout;
}

PixelOut PS(VertexOut pin)
{
	PixelOut pout = (PixelOut)0.0f;

	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	float3 specular = matData.mSpecular;
	float roughness = matData.mRoughness;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	uint normalMapIndex = matData.mNormalMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuseAlbedo.a - 0.1f);

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
	pin.mNormalW = normalize(pin.mNormalW);
	pin.mTangentW = normalize(pin.mTangentW);
	pin.mBinormalW = normalize(pin.mBinormalW);
	float3 bumpedNormalW = pin.mNormalW;

	// ��ָʿ��� Tangent Space�� ����� World Space�� ��ַ� ��ȯ�Ѵ�.
	if (normalMapIndex != DISABLED)
	{
		float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
		float3 normalT = 2.0f * normalMapSample.rgb - 1.0f;
		float3x3 tbn = float3x3(pin.mTangentW, pin.mBinormalW, pin.mNormalW);
		bumpedNormalW = mul(normalT, tbn);
		bumpedNormalW = normalize(bumpedNormalW);
	}

	pout.mDiffuse = diffuseAlbedo;
	pout.mSpecularAndRoughness = float4(specular, roughness);
	pout.mNormal = float4(bumpedNormalW, 1.0f);
	pout.mPosition = float4(pin.mPosW, 1.0f);

	return pout;
}