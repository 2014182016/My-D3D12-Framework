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

	// �� ������ ����� Material�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
	
    // World Space�� ��ȯ�Ѵ�.
    float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);
    vout.mPosW = posW.xyz;

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.mPosH = mul(posW, gViewProj);

	// World Matrix�� ��յ� ��ʰ� ���ٰ� �����ϰ� Normal�� ��ȯ�Ѵ�.
	// ��յ� ��ʰ� �ִٸ� ����ġ ����� ����ؾ� �Ѵ�.
    vout.mNormalW = mul(vin.mNormalL, (float3x3)gObjWorld);
	vout.mTangentW = mul(vin.mTangentU, (float3x3)gObjWorld);
	vout.mBinormalW = mul(vin.mBinormalU, (float3x3)gObjWorld);
	
	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), matData.mMatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// �� �ȼ��� ����� Material Data�� �����´�.
	MaterialData matData = gMaterialData[gObjMaterialIndex];
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

    // �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
    clip(diffuseAlbedo.a - 0.1f);

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

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, specular, shininess };

	// Lighting�� �ǽ��Ѵ�.
    float4 directLight = ComputeShadowLighting(gLights, mat, pin.mPosW, bumpedNormalW, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	float4 ssaoPosH = mul(pin.mPosW, gViewProjTex);
	ssaoPosH /= ssaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy, 0.0f).r;
	litColor *= ambientAccess;
#endif

	// �л� �������� ���ĸ� �����´�.
	litColor.a = diffuseAlbedo.a;


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
		float fogAmount;

		switch(gFogType)
		{
		case FOG_LINEAR:
			// �Ÿ��� ���� �Ȱ� ������ ���� ����� ����Ѵ�.
			fogAmount = saturate((distToEye - gFogStart) / gFogRange);
			litColor = lerp(litColor, gFogColor, fogAmount);
			break;
		case FOG_EXPONENTIAL:
			// ���������� �ָ� �������� �Ȱ��� �� �β�������.
			fogAmount = exp(-distToEye * gFogDensity);
			litColor = lerp(gFogColor, litColor, fogAmount);
			break;
		case FOG_EXPONENTIAL2:
			// �ſ� �β��� �Ȱ��� ��Ÿ����.
			fogAmount = exp(-pow(distToEye * gFogDensity, 2));
			litColor = lerp(gFogColor, litColor, fogAmount);
			break;
		}
	}

    return litColor;
}


