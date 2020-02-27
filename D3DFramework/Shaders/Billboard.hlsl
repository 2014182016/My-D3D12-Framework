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
	float3 mPosW      : POSITION0;
	float3 mNormal    : NORMAL;
	float2 mTexC      : TEXCOORD;
};

struct PixelOut
{
	float4 mDiffuse  : SV_TARGET0;
	float4 mSpecularRoughness : SV_TARGET1;
	float4 mPosition : SV_TARGET2;
	float4 mNormal   : SV_TARGET3;
	float4 mNormalx  : SV_TARGET4;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// 자료를 그대로 기하 셰이더에 넘겨준다.
	vout.mCenterW = vin.mPosW;
	vout.mSizeW = vin.mSizeW;

	return vout;
}

// 각 점을 사각형(정점 4개)으로 확장하므로 기하 셰이더 호출당
// 최대 출력 정점 개수는 4이다.
[maxvertexcount(4)]
void GS(point VertexOut gin[1], // Primitive는 Point이므로 들어오는 정점은 하나이다.
	inout TriangleStream<GeoOut> triStream) 
{
	// 빌보드가 xz 평면에 붙어서 y 방향으로 세워진 상태에서 카메라를
	// 향하게 만드는 세계 공간 기준 빌보드 지역 좌표계를 계산한다.
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].mCenterW;
	look.y = 0.0f; // y 축 정렬이므로 sz평면에 투영
	look = normalize(look);
	float3 right = cross(up, look); 

	// 세계 공간 기준의 삼각형 띠 정점들(사각형을 구성하는)을 계산한다.
	float halfWidth = 0.5f*gin[0].mSizeW.x;
	float halfHeight = 0.5f*gin[0].mSizeW.y;

	float4 v[4];
	v[0] = float4(gin[0].mCenterW + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin[0].mCenterW + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin[0].mCenterW - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin[0].mCenterW - halfWidth * right + halfHeight * up, 1.0f);

	// 사각형 정점들을 세계  공간으로 변환하고, 
	// 그것들을 하나의 삼각형 띠로 출력한다.
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
		gout.mNormal = look;
		gout.mTexC = texC[i];

		triStream.Append(gout);
	}
}

PixelOut PS(GeoOut pin)
{
	PixelOut pout = (PixelOut)0.0f;

	float4 diffuse; float3 specular; float roughness;
	GetMaterialAttibute(gObjMaterialIndex, diffuse, specular, roughness);
	diffuse *= GetDiffuseMapSample(gObjMaterialIndex, pin.mTexC);

	// 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
	clip(diffuse.a - 0.1f);

	// 법선을 보간하면 단위 길이가 아니게 될 수 있으므로 다시 정규화한다.
	pin.mNormal = normalize(pin.mNormal);

	pout.mDiffuse = diffuse;
	pout.mSpecularRoughness = float4(specular, roughness);
	pout.mNormal = float4(pin.mNormal, 1.0f);
	pout.mNormalx = float4(pin.mNormal, 1.0f);
	pout.mPosition = float4(pin.mPosW, 1.0f);

	return pout;
}


