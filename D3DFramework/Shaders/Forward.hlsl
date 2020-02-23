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

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// 이 정점에 사용할 Material을 가져온다.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	
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
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// 이 픽셀에 사용할 Material Data를 가져온다.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	float3 specular = matData.mSpecular;
	float roughness = matData.mRoughness;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	uint normalMapIndex = matData.mNormalMapIndex;

	// 텍스처 배열의 텍스처를 동적으로 조회한다.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

    // 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
    clip(diffuseAlbedo.a - 0.1f);

	// 법선을 보간하면 단위 길이가 아니게 될 수 있으므로 다시 정규화한다.
    pin.mNormalW = normalize(pin.mNormalW);
	pin.mTangentW = normalize(pin.mTangentW);
	pin.mBinormalW = normalize(pin.mBinormalW);
	float3 bumpedNormalW = pin.mNormalW;
	
	// 노멀맵에서 Tangent Space의 노멀을 World Space의 노멀로 변환한다.
	if (normalMapIndex != DISABLED)
	{
		float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
		float3 normalT = 2.0f * normalMapSample.rgb - 1.0f;
		float3x3 tbn = float3x3(pin.mTangentW, pin.mBinormalW, pin.mNormalW);
		bumpedNormalW = mul(normalT, tbn);
		bumpedNormalW = normalize(bumpedNormalW);
	}

    // 조명되는 픽셀에서 눈으로의 벡터
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse를 전반적으로 밝혀주는 Ambient항
	float4 ambient = gAmbientLight * diffuseAlbedo;

	// roughness와 normal를 이용하여 shininess를 계산한다.
	const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, specular, shininess };

	// Lighting을 실시한다.
    float4 directLight = ComputeShadowLighting(gLights, mat, pin.mPosW, bumpedNormalW, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	float4 ssaoPosH = mul(pin.mPosW, gViewProjTex);
	ssaoPosH /= ssaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy, 0.0f).r;
	litColor *= ambientAccess;
#endif

	// 분산 재질에서 알파를 가져온다.
	litColor.a = diffuseAlbedo.a;


#ifdef SKY_REFLECTION
	// Sky Cube Map으로 환경 매핑을 사용하여 픽셀에 입힌다.
	if (gCurrentSkyCubeMapIndex != DISABLED)
	{
		float3 r = reflect(-toEyeW, pin.mNormalW);
		float4 reflectionColor = gCubeMaps[gCurrentSkyCubeMapIndex].Sample(gsamLinearWrap, r);
		float3 fresnelFactor = SchlickFresnel(specular, pin.mNormalW, r);
		litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
	}
#endif

	if (gFogEnabled)
	{
		float fogAmount;

		switch(gFogType)
		{
		case FOG_LINEAR:
			// 거리에 따라 안개 색상을 선형 감쇠로 계산한다.
			fogAmount = saturate((distToEye - gFogStart) / gFogRange);
			litColor = lerp(litColor, gFogColor, fogAmount);
			break;
		case FOG_EXPONENTIAL:
			// 지수적으로 멀리 있을수록 안개가 더 두꺼워진다.
			fogAmount = exp(-distToEye * gFogDensity);
			litColor = lerp(gFogColor, litColor, fogAmount);
			break;
		case FOG_EXPONENTIAL2:
			// 매우 두꺼운 안개를 나타낸다.
			fogAmount = exp(-pow(distToEye * gFogDensity, 2));
			litColor = lerp(gFogColor, litColor, fogAmount);
			break;
		}
	}

    return litColor;
}


