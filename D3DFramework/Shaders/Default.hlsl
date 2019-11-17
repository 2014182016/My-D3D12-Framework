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
	
	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.mDiffuseAlbedo;
	float3 specular = matData.mSpecular;
	float  roughness = matData.mRoughness;
	uint diffuseMapIndex = matData.mDiffuseMapIndex;
	uint normalMapIndex = matData.mNormalMapIndex;
	
	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != -1)
	{
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
	}

#ifdef ALPHA_TEST
    // �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
    clip(diffuseAlbedo.a - 0.1f);
#endif

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
    pin.mNormalW = normalize(pin.mNormalW);
	float3 bumpedNormalW = pin.mNormalW;;
	
	// ��ָʿ��� Local Space�� ����� World Space�� ��ַ� ��ȯ�Ѵ�.
	if (normalMapIndex != -1)
	{
		float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC);
		bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.mNormalW, pin.mTangentW);
	}

    // ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

    // Diffuse�� ���������� �����ִ� Ambient��
    float4 ambient = gAmbientLight*diffuseAlbedo;

	// �ٸ� ��ü�� ������ ������ ���� shadowFactor�� �ȼ��� ��Ӱ� �Ѵ�.
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
    const float shininess = 1.0f - roughness;

	// Lighting�� �ǽ��Ѵ�.
    Material mat = { diffuseAlbedo, specular, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.mPosW,
        bumpedNormalW, toEyeW, shadowFactor);
	float4 litColor = ambient + directLight;

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


