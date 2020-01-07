#include "Common.hlsl"

struct VertexIn
{
	float3 mPosW  : POSITION;
	float4 mColor : COLOR;
	float3 mNormal: NORMAL;
	float2 mSize  : SIZE;
};

struct VertexOut
{
	float3 mPosW  : POSITION;
	float4 mColor : COLOR;
	float3 mNormal: NORMAL;
	float2 mSize  : SIZE;
};

struct GeoOut
{
	float4 mPosH      : SV_POSITION;
	float3 mPosW      : POSITION;
	float4 mColor	  : COLOR;
	float3 mNormalW   : NORMAL;
	float2 mTexC      : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.mPosW = vin.mPosW;
	vout.mColor = vin.mColor;
	vout.mNormal = vin.mNormal;
	vout.mSize = vin.mSize;

	return vout;
}

// �� ���� �簢��(���� 4��)���� Ȯ���ϹǷ� ���� ���̴� ȣ���
// �ִ� ��� ���� ������ 4�̴�.
[maxvertexcount(4)]
void GS(point VertexOut gin[1], // Primitive�� Point�̹Ƿ� ������ ������ �ϳ��̴�.
	inout TriangleStream<GeoOut> triStream)
{
	MaterialData matData = gMaterialData[gParticleMaterialIndex];

	float4 v[4];

	// ���� ���� ������ �ﰢ�� �� ������(�簢���� �����ϴ�)�� ����Ѵ�.
	float halfWidth = 0.5f * gin[0].mSize.x;
	float halfHeight = 0.5f * gin[0].mSize.y;

	const float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look;
	float3 right;

	if (gParticleFacingCamera)
	{
		// �����尡 xz ��鿡 �پ y �������� ������ ���¿��� ī�޶�
		// ���ϰ� ����� ���� ���� ���� ������ ���� ��ǥ�踦 ����Ѵ�.
		look = gEyePosW - gin[0].mPosW;
		look.y = 0.0f; // y �� �����̹Ƿ� sz��鿡 ����
		look = normalize(look);
		right = cross(up, look);
	}
	else
	{
		look = gin[0].mNormal;
		right = cross(up, look);
	}

	v[0] = float4(gin[0].mPosW + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin[0].mPosW + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin[0].mPosW - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin[0].mPosW - halfWidth * right + halfHeight * up, 1.0f);

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
		gout.mColor = gin[0].mColor;
		gout.mNormalW = look;
		gout.mTexC = mul(float4(texC[i], 0.0f, 1.0f), matData.mMatTransform).xy;

		triStream.Append(gout);
	}
}

float4 PS(GeoOut pin) : SV_Target
{
	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gParticleMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuseAlbedo.a - 0.1f);

	return diffuseAlbedo * pin.mColor;
}


