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
	float4 mSpecularRoughness : SV_TARGET1;
	float4 mPosition : SV_TARGET2;
	float4 mNormal   : SV_TARGET3;
	float4 mNormalx  : SV_TARGET4;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// World Space로 변환한다.
	float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);
	vout.mPosW = posW.xyz;

	// 동차 절단 공간으로 변환한다.
	vout.mPosH = mul(posW, gViewProj);

	// World Matrix에 비균등 비례가 없다고 가정하고 Normal을 변환한다.
	// 비균등 비례가 있다면 역전치 행렬을 사용해야 한다.
	vout.mNormalW = mul(vin.mNormalL, (float3x3)gObjWorld);
	vout.mTangentW = mul(vin.mTangentU, (float3x3)gObjWorld);
	vout.mBinormalW = mul(vin.mBinormalU, (float3x3)gObjWorld);

	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

PixelOut PS(VertexOut pin)
{
	PixelOut pout = (PixelOut)0.0f;

	float4 diffuse = 0.0f; float3 specular = 0.0f; float roughness = 0.0f;
	GetMaterialAttibute(gObjMaterialIndex, diffuse, specular, roughness);

	diffuse *= GetDiffuseMapSample(gObjMaterialIndex, pin.mTexC);

	// 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
	clip(diffuse.a - 0.1f);

	// 법선을 보간하면 단위 길이가 아니게 될 수 있으므로 다시 정규화한다.
	pin.mNormalW = normalize(pin.mNormalW);
	pin.mTangentW = normalize(pin.mTangentW);
	pin.mBinormalW = normalize(pin.mBinormalW);

	// 노멀맵을 사용하지 않은 노멀을 저장한다.
	pout.mNormalx = float4(pin.mNormalW, 1.0f);

	float3 bumpedNormalW = pin.mNormalW;

	float4 normalMapSample = GetNormalMapSample(gObjMaterialIndex, pin.mTexC);
	// 노멀맵에서 Tangent Space의 노멀을 World Space의 노멀로 변환한다.
	if (any(normalMapSample))
	{
		// 노멀맵에서 Tangent Space의 노멀을 World Space의 노멀로 변환한다.
		float3 normalT = 2.0f * normalMapSample.rgb - 1.0f;
		float3x3 tbn = float3x3(pin.mTangentW, pin.mBinormalW, pin.mNormalW);
		bumpedNormalW = mul(normalT, tbn);
		bumpedNormalW = normalize(bumpedNormalW);
	}

	pout.mDiffuse = float4(diffuse.rgb, 1.0f);
	pout.mSpecularRoughness = float4(specular, roughness);
	pout.mPosition = float4(pin.mPosW, 1.0f);
	pout.mNormal = float4(bumpedNormalW, 1.0f);

	return pout;
}