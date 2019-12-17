//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#ifndef LIGHT_NUM
#define LIGHT_NUM 1
#endif

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

struct Light
{
	float4x4 mWorld;
	float4x4 mViewProj;
	float4x4 mShadowTransform;
    float3 mStrength;
    float mFalloffStart; // point/spot light only
    float3 mDirection;   // directional/spot light only
    float mFalloffEnd;   // point/spot light only
    float3 mPosition;    // point light only
    float mSpotPower;    // spot light only
	bool mEnabled;
	bool mSelected;
	uint mType;
	float mPadding0;
};

struct Material
{
    float4 mDiffuseAlbedo;
    float3 mSpecular;
    float mShininess;
};

// ������ �������� �����ϴ� ���� ���� ����� ����Ѵ�.
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// ������ �������� ���� �ٻ縦 ���Ѵ�.
// ��, ������ n�� ǥ�鿡�� ������ ȿ���� ���� �ݻ�Ǵ� ���� ������
// �� ���� L�� ǥ�� ���� n ������ ������ �ٰ��ؼ� �ٻ��Ѵ�.
float3 SchlickFresnel(float3 r0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = r0 + (1.0f - r0)*(pow(f0, 5));

    return reflectPercent;
}

// ���� ������ �ݻ籤�� ���� ����Ѵ�.
// ��, �л� �ݻ�� �ݿ� �ݻ��� ���� ���Ѵ�.
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.mShininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 specularFactor = SchlickFresnel(mat.mSpecular, halfVec, lightVec);

    float3 specAlbedo = specularFactor * roughnessFactor;

    // �ݿ� �ݻ��� ������ [0,1] ���� �ٱ��� ���� �� ���� ������,
	// �츮�� LDR �������� �����ϹǷ�, �ݻ����� 1�̸����� �����.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.mDiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material mat, float3 normal, float3 toEye)
{
    // �� ���ʹ� �������� ���ư��� ������ �ݴ� ������ ����Ų��.
    float3 lightVec = -light.mDirection;

    // ����Ʈ�� �ڻ��� ��Ģ�� ���� ���� ���⸦ ���δ�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.mStrength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // ǥ�鿡�� ���������� ����
    float3 lightVec = light.mPosition - pos;

    // ������ ǥ�� ������ �Ÿ�
    float d = length(lightVec);

    //���� ����
    if(d > light.mFalloffEnd)
		return float3(0.0f, 0.0f, 0.0f);

    // �� ���͸� ����ȭ�Ѵ�.
    lightVec /= d;

    // ����Ʈ�� �ڻ��� ��Ģ�� ���� ���� ���⸦ ���δ�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.mStrength * ndotl;

    // �Ÿ��� ���� ���� �����Ѵ�.
    float att = CalcAttenuation(d, light.mFalloffStart, light.mFalloffEnd);
    lightStrength = lightStrength * att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // ǥ�鿡�� ���������� ����
    float3 lightVec = light.mPosition - pos;

    // ������ ǥ�� ������ �Ÿ�
    float d = length(lightVec);

    // ���� ����
    if(d > light.mFalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    // �� ���͸� ����ȭ�Ѵ�.
    lightVec /= d;

    // ����Ʈ�� �ڻ��� ��Ģ�� ���� ���� ���⸦ ���δ�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.mStrength * ndotl;

    // �Ÿ��� ���� ���� �����Ѵ�.
    float att = CalcAttenuation(d, light.mFalloffStart, light.mFalloffEnd);
    lightStrength *= att;

    // ������ ����� ����Ѵ�.
    float spotFactor = pow(max(dot(-lightVec, light.mDirection), 0.0f), light.mSpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(StructuredBuffer<Light> lights, Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;

	for (uint i = 0; i < LIGHT_NUM; ++i)
	{
		if (lights[i].mEnabled == 0)
			continue;

		switch (lights[i].mType)
		{
		case DIRECTIONAL_LIGHT:
			result += shadowFactor[i] * ComputeDirectionalLight(lights[i], mat, normal, toEye);
			break;
		case POINT_LIGHT:
			result += ComputePointLight(lights[i], mat, pos, normal, toEye);
			break;
		case SPOT_LIGHT:
			result += ComputeSpotLight(lights[i], mat, pos, normal, toEye);
			break;
		}
	}

    return float4(result, 0.0f);
}