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

	// World Space로 변환한다.
	float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);
	vout.mPosW = posW.xyz;

	// z = w이면 해당 픽셀은 항상 far plane에 위치해있다.
	vout.mPosH = mul(posW, gViewProj).xyww;

	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

[earlydepthstencil]
float4 PS(VertexOut pin) : SV_Target
{
	// 이 픽셀에 사용할 Material Data를 가져온다.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	float diffuseIntensity = 1.0f;

	// 텍스처 배열의 텍스처를 동적으로 조회한다.
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

