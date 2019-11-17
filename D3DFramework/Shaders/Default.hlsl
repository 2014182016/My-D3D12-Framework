//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 mPosL     : POSITION;
    float3 mNormalL  : NORMAL;
	float3 mTangentU : TANGENT;
	float2 mTexC     : TEXCOORD;
};

struct VertexOut
{
	float4 mPosH     : SV_POSITION;
    float3 mPosW     : POSITION;
    float3 mNormalW  : NORMAL;
	float3 mTangentW : TANGENT;
	float2 mTexC     : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// 이 정점에 사용할 Material을 가져온다.
	MaterialData matData = gMaterialData[gMaterialIndex];
	
    // World Space로 변환한다.
    float4 posW = mul(float4(vin.mPosL, 1.0f), gWorld);
    vout.mPosW = posW.xyz;

	// 동차 절단 공간으로 변환한다.
	vout.mPosH = mul(posW, gViewProj);

	// World Matrix에 비균등 비례가 없다고 가정하고 Normal을 변환한다.
	// 비균등 비례가 있다면 역전치 행렬을 사용해야 한다.
    vout.mNormalW = mul(vin.mNormalL, (float3x3)gWorld);
	vout.mTangentW = mul(vin.mTangentU, (float3x3)gWorld);
	
	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// 이 픽셀에 사용할 Material Data를 가져온다.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	float3 specular = matData.mSpecular;
	float  roughness = matData.mRoughness;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	uint normalMapIndex = matData.mNormalMapIndex;
	
	// 텍스처 배열의 텍스처를 동적으로 조회한다.
	if (diffuseMapIndex != -1)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

#ifdef ALPHA_TEST
    // 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
    clip(diffuseAlbedo.a - 0.1f);
#endif

	// 법선을 보간하면 단위 길이가 아니게 될 수 있으므로 다시 정규화한다.
    pin.mNormalW = normalize(pin.mNormalW);
	float3 bumpedNormalW = pin.mNormalW;;
	
	// 노멀맵에서 Local Space의 노멀을 World Space의 노멀로 변환한다.
	if (normalMapIndex != -1)
	{
		float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
		bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.mNormalW, pin.mTangentW);
	}

    // 조명되는 픽셀에서 눈으로의 벡터
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

    // Diffuse를 전반적으로 밝혀주는 Ambient항
    float4 ambient = gAmbientLight*diffuseAlbedo;

	// 다른 물체에 가려진 정도에 따라 shadowFactor로 픽셀을 어둡게 한다.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);

	// roughness와 normal를 이용하여 shininess를 계산한다.
    const float shininess = 1.0f - roughness;

	// Lighting을 실시한다.
    Material mat = { diffuseAlbedo, specular, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.mPosW,
        bumpedNormalW, toEyeW, shadowFactor);
	float4 litColor = ambient + directLight;

	if (gFogEnabled)
	{
		// 거리에 따라 안개 색상을 선형 감쇠로 계산한다.
		float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
		litColor = lerp(litColor, gFogColor, fogAmount);
	}
	
    // 분산 재질에서 알파를 가져온다.
    litColor.a = diffuseAlbedo.a;

    return litColor;
}


