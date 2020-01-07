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

// 각 점을 사각형(정점 4개)으로 확장하므로 기하 셰이더 호출당
// 최대 출력 정점 개수는 4이다.
[maxvertexcount(4)]
void GS(point VertexOut gin[1], // Primitive는 Point이므로 들어오는 정점은 하나이다.
	inout TriangleStream<GeoOut> triStream)
{
	MaterialData matData = gMaterialData[gParticleMaterialIndex];

	float4 v[4];

	// 세계 공간 기준의 삼각형 띠 정점들(사각형을 구성하는)을 계산한다.
	float halfWidth = 0.5f * gin[0].mSize.x;
	float halfHeight = 0.5f * gin[0].mSize.y;

	const float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look;
	float3 right;

	if (gParticleFacingCamera)
	{
		// 빌보드가 xz 평면에 붙어서 y 방향으로 세워진 상태에서 카메라를
		// 향하게 만드는 세계 공간 기준 빌보드 지역 좌표계를 계산한다.
		look = gEyePosW - gin[0].mPosW;
		look.y = 0.0f; // y 축 정렬이므로 sz평면에 투영
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
	// 이 픽셀에 사용할 Material Data를 가져온다.
	MaterialData matData = gMaterialData[gParticleMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;

	// 텍스처 배열의 텍스처를 동적으로 조회한다.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

	// 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
	clip(diffuseAlbedo.a - 0.1f);

	return diffuseAlbedo * pin.mColor;
}


