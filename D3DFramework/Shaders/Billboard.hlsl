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
	float4 mPosH    : SV_POSITION;
	float3 mPosW    : POSITION;
	float3 mNormalW : NORMAL;
	float2 mTexC    : TEXCOORD;
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
		gout.mPosW = v[i].xyz;
		gout.mNormalW = look;
		gout.mTexC = texC[i];

		triStream.Append(gout);
	}
}

float4 PS(GeoOut pin) : SV_Target
{
	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	float3 specular = matData.mSpecular;
	float  roughness = matData.mRoughness;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	uint normalMapIndex = matData.mNormalMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);

#ifdef ALPHA_TEST
	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuseAlbedo.a - 0.1f);
#endif

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
	pin.mNormalW = normalize(pin.mNormalW);

	// ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse�� ���������� �����ִ� Ambient��
	float4 ambient = gAmbientLight * diffuseAlbedo;

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;

	// Lighting�� �ǽ��Ѵ�.
	Material mat = { diffuseAlbedo, specular, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.mPosW,
		pin.mNormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

#ifdef FOG
	// �Ÿ��� ���� �Ȱ� ������ ���� ����� ����Ѵ�.
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

	litColor.a = diffuseAlbedo.a;

	return litColor;
}


