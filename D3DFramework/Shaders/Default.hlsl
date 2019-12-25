//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

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
	float4 mShadowPosH: POSITION0;
    float3 mPosW      : POSITION1;
    float3 mNormalW   : NORMAL;
	float3 mTangentW  : TANGENT;
	float3 mBinormalW : BINORMAL;
	float2 mTexC      : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// �� ������ ����� Material�� �����´�.
	MaterialData matData = gMaterialData[gMaterialIndex];
	
    // World Space�� ��ȯ�Ѵ�.
    float4 posW = mul(float4(vin.mPosL, 1.0f), gWorld);
    vout.mPosW = posW.xyz;

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.mPosH = mul(posW, gViewProj);

	// World Matrix�� ��յ� ��ʰ� ���ٰ� �����ϰ� Normal�� ��ȯ�Ѵ�.
	// ��յ� ��ʰ� �ִٸ� ����ġ ����� ����ؾ� �Ѵ�.
    vout.mNormalW = mul(vin.mNormalL, (float3x3)gWorld);
	vout.mTangentW = mul(vin.mTangentU, (float3x3)gWorld);
	vout.mBinormalW = mul(vin.mBinormalU, (float3x3)gWorld);
	
	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;

	// ���� ������ ������ �ؽ�ó �������� ��ȯ�Ѵ�.
	vout.mShadowPosH = mul(posW, gLights[0].mShadowTransform);
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	float3 specular = matData.mSpecular;
	float roughness = matData.mRoughness;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	uint normalMapIndex = matData.mNormalMapIndex;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

#ifdef ALPHA_TEST
    // �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
    clip(diffuseAlbedo.a - 0.1f);
#endif

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
    pin.mNormalW = normalize(pin.mNormalW);
	pin.mTangentW = normalize(pin.mTangentW);
	pin.mBinormalW = normalize(pin.mBinormalW);
	float3 bumpedNormalW = pin.mNormalW;
	
	// ��ָʿ��� Tangent Space�� ����� World Space�� ��ַ� ��ȯ�Ѵ�.
	if (normalMapIndex != DISABLED)
	{
		float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
		float3 normalT = 2.0f * normalMapSample.rgb - 1.0f;
		float3x3 tbn = float3x3(pin.mTangentW, pin.mBinormalW, pin.mNormalW);
		bumpedNormalW = mul(normalT, tbn);
		bumpedNormalW = normalize(bumpedNormalW);
	}

    // ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

    // Diffuse�� ���������� �����ִ� Ambient��
    float4 ambient = gAmbientLight * diffuseAlbedo;

	// �ٸ� ��ü�� ������ ������ ���� shadowFactor�� �ȼ��� ��Ӱ� �Ѵ�.
	float3 shadowFactor = 1.0f;
	shadowFactor = CalcShadowFactor(pin.mShadowPosH, 0);

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;

    Material mat = { diffuseAlbedo, specular, shininess };

	// Lighting�� �ǽ��Ѵ�.
    float4 directLight = ComputeLighting(gLights, mat, pin.mPosW,
        bumpedNormalW, toEyeW, shadowFactor);
	float4 litColor = ambient + directLight;


#ifdef SKY_REFLECTION
	// Sky Cube Map���� ȯ�� ������ ����Ͽ� �ȼ��� ������.
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
		// �Ÿ��� ���� �Ȱ� ������ ���� ����� ����Ѵ�.
		float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
		litColor = lerp(litColor, gFogColor, fogAmount);
	}

    // �л� �������� ���ĸ� �����´�.
    litColor.a = diffuseAlbedo.a;

    return litColor;
}


