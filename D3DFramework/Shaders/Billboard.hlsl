//***************************************************************************************
// TreeSprite.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

static const float2 gTexC[4] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 1.0f),
	float2(1.0f, 0.0f)
};

struct VertexIn
{
	float3 posW  : POSITION;
	float2 sizeW : SIZE;
};

struct VertexOut
{
	float3 centerW : POSITION;
	float2 sizeW   : SIZE;
};

struct GeoOut
{
	float4 posH      : SV_POSITION;
	float3 posW      : POSITION0;
	float3 normal    : NORMAL;
	float2 texC      : TEXCOORD;
};

struct PixelOut
{
	float4 diffuse  : SV_TARGET0;
	float4 specularRoughness : SV_TARGET1;
	float4 position : SV_TARGET2;
	float4 normal   : SV_TARGET3;
	float4 normalx  : SV_TARGET4;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// �ڷḦ �״�� ���� ���̴��� �Ѱ��ش�.
	vout.centerW = vin.posW;
	vout.sizeW = vin.sizeW;

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
	float3 look = gEyePosW - gin[0].centerW;
	look.y = 0.0f; // y �� �����̹Ƿ� sz��鿡 ����
	look = normalize(look);
	float3 right = cross(up, look); 

	// ���� ���� ������ �ﰢ�� �� ������(�簢���� �����ϴ�)�� ����Ѵ�.
	float halfWidth = 0.5f*gin[0].sizeW.x;
	float halfHeight = 0.5f*gin[0].sizeW.y;

	// �� ������ ���� ��ǥ�� ����Ѵ�.
	float4 v[4];
	v[0] = float4(gin[0].centerW + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin[0].centerW + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin[0].centerW - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin[0].centerW - halfWidth * right + halfHeight * up, 1.0f);

	GeoOut gout;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gout.posH = mul(v[i], gViewProj);
		gout.posW = v[i].xyz;
		gout.normal = look;
		gout.texC = mul(float4(gTexC[i], 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

		triStream.Append(gout);
	}
}

PixelOut PS(GeoOut pin)
{
	PixelOut pout = (PixelOut)0.0f;

	// �� �ȼ��� ����� ���͸��� �����͸� �����´�.
	MaterialData materialData = gMaterialData[gObjMaterialIndex];

	float4 diffuse = materialData.diffuseAlbedo;
	float3 specular = materialData.specular;
	float roughness = materialData.roughness;
	uint diffuseMapIndex = materialData.diffuseMapIndex;

	if(diffuseMapIndex != DISABLED)
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC);

	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuse.a - 0.1f);

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
	pin.normal = normalize(pin.normal);

	// ���� ��Ƽ ���������� G���۸� ä���.
	pout.diffuse = float4(diffuse.rgb, 1.0f);
	pout.specularRoughness = float4(specular, roughness);
	pout.normal = float4(pin.normal, 1.0f);
	pout.normalx = float4(pin.normal, 1.0f);
	pout.position = float4(pin.posW, 1.0f);

	return pout;
}


