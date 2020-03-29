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

	// 자료를 그대로 기하 셰이더에 넘겨준다.
	vout.centerW = vin.posW;
	vout.sizeW = vin.sizeW;

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
	float3 look = gEyePosW - gin[0].centerW;
	look.y = 0.0f; // y 축 정렬이므로 sz평면에 투영
	look = normalize(look);
	float3 right = cross(up, look); 

	// 세계 공간 기준의 삼각형 띠 정점들(사각형을 구성하는)을 계산한다.
	float halfWidth = 0.5f*gin[0].sizeW.x;
	float halfHeight = 0.5f*gin[0].sizeW.y;

	// 각 정점의 월드 좌표를 계산한다.
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

	// 이 픽셀에 사용할 머터리얼 데이터를 가져온다.
	MaterialData materialData = gMaterialData[gObjMaterialIndex];

	float4 diffuse = materialData.diffuseAlbedo;
	float3 specular = materialData.specular;
	float roughness = materialData.roughness;
	uint diffuseMapIndex = materialData.diffuseMapIndex;

	if(diffuseMapIndex != DISABLED)
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC);

	// 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
	clip(diffuse.a - 0.1f);

	// 법선을 보간하면 단위 길이가 아니게 될 수 있으므로 다시 정규화한다.
	pin.normal = normalize(pin.normal);

	// 다중 멀티 렌더링으로 G버퍼를 채운다.
	pout.diffuse = float4(diffuse.rgb, 1.0f);
	pout.specularRoughness = float4(specular, roughness);
	pout.normal = float4(pin.normal, 1.0f);
	pout.normalx = float4(pin.normal, 1.0f);
	pout.position = float4(pin.posW, 1.0f);

	return pout;
}


