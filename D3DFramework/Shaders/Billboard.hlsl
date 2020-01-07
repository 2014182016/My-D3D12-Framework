//***************************************************************************************
// TreeSprite.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 mPosW  : POSITION;
	float2 mSizeW : SIZE;
};

struct VertexOut
{
	float3 mCenterW : POSITION;
	float2 mSizeW   : SIZE;
};

struct GeoOut
{
	float4 mPosH      : SV_POSITION;
	float4 mPosW      : POSITION0;
	float3 mNormal    : NORMAL;
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
	VertexOut vout;

	// �ڷḦ �״�� ���� ���̴��� �Ѱ��ش�.
	vout.mCenterW = vin.mPosW;
	vout.mSizeW = vin.mSizeW;

	return vout;
}

// �� ���� �簢��(���� 4��)���� Ȯ���ϹǷ� ���� ���̴� ȣ���
// �ִ� ��� ���� ������ 4�̴�.
[maxvertexcount(4)]
void GS(point VertexOut gin[1], // Primitive�� Point�̹Ƿ� ������ ������ �ϳ��̴�.
	inout TriangleStream<GeoOut> triStream) 
{
	// �����尡 xz ��鿡 �پ y �������� ������ ���¿��� ī�޶�
	// ���ϰ� ����� ���� ���� ���� ������ ���� ��ǥ�踦 ����Ѵ�.
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].mCenterW;
	look.y = 0.0f; // y �� �����̹Ƿ� sz��鿡 ����
	look = normalize(look);
	float3 right = cross(up, look); 

	// ���� ���� ������ �ﰢ�� �� ������(�簢���� �����ϴ�)�� ����Ѵ�.
	float halfWidth = 0.5f*gin[0].mSizeW.x;
	float halfHeight = 0.5f*gin[0].mSizeW.y;

	float4 v[4];
	v[0] = float4(gin[0].mCenterW + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin[0].mCenterW + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin[0].mCenterW - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin[0].mCenterW - halfWidth * right + halfHeight * up, 1.0f);

	// �簢�� �������� ����  �������� ��ȯ�ϰ�, 
	// �װ͵��� �ϳ��� �ﰢ�� ��� ����Ѵ�.
	float2 texC[4] =
	{
		float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
	};

	GeoOut gout;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gout.mPosH = mul(v[i], gViewProj);
		gout.mPosW = v[i];
		gout.mNormal = look;
		gout.mTexC = texC[i];

		triStream.Append(gout);
	}
}

PixelOut PS(GeoOut pin) : SV_Target
{
	PixelOut pout = (PixelOut)0.0f;

	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	float3 specular = matData.mSpecular;
	float roughness = matData.mRoughness;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuseAlbedo.a - 0.1f);

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
	pin.mNormal = normalize(pin.mNormal);

	pout.mDiffuse = diffuseAlbedo;
	pout.mSpecularAndRoughness = float4(specular, roughness);
	pout.mNormal = float4(pin.mNormal, 1.0f);
	pout.mPosition = pin.mPosW;

	return pout;
}


