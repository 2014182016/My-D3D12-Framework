//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#include "Defines.hlsl"
#include "Material.hlsl"

struct Light
{
	float4x4 shadowTransform;
    float3 strength;
    float falloffStart;
    float3 direction;   
    float falloffEnd;   
    float3 position;    
    float spotAngle;   
	bool enabled;
	bool selected;
	uint type;
	float padding0;
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
    const float m = mat.shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 specularFactor = SchlickFresnel(mat.specular, halfVec, lightVec);

    float3 specAlbedo = specularFactor * roughnessFactor;

    // �ݿ� �ݻ��� ������ [0,1] ���� �ٱ��� ���� �� ���� ������,
	// �츮�� LDR �������� �����ϹǷ�, �ݻ����� 1�̸����� �����.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.diffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material mat, float3 normal, float3 toEye)
{
    // �� ���ʹ� �������� ���ư��� ������ �ݴ� ������ ����Ų��.
    float3 lightVec = -light.direction;

    // ����Ʈ�� �ڻ��� ��Ģ�� ���� ���� ���⸦ ���δ�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // ǥ�鿡�� ���������� ����
    float3 lightVec = light.position - pos;

    // ������ ǥ�� ������ �Ÿ�
    float d = length(lightVec);

    //���� ����
    if(d > light.falloffEnd)
		return float3(0.0f, 0.0f, 0.0f);

    // �� ���͸� ����ȭ�Ѵ�.
    lightVec /= d;

    // ����Ʈ�� �ڻ��� ��Ģ�� ���� ���� ���⸦ ���δ�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;

    // �Ÿ��� ���� ���� �����Ѵ�.
    float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
    lightStrength = lightStrength * att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // ǥ�鿡�� ���������� ����
    float3 lightVec = light.position - pos;

    // ������ ǥ�� ������ �Ÿ�
    float d = length(lightVec);

    // ���� ����
	if (d > light.falloffEnd)
		return float3(0.0f, 0.0f, 0.0f);

    // �� ���͸� ����ȭ�Ѵ�.
    lightVec /= d;

    // ����Ʈ�� �ڻ��� ��Ģ�� ���� ���� ���⸦ ���δ�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;

    // �Ÿ��� ���� ���� �����Ѵ�.
    float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
    lightStrength *= att;

	float minCos = cos(radians(light.spotAngle));
	float maxCos = lerp(minCos, 1.0f, 0.5f);
	float cosAngle = max(dot(-lightVec, light.direction), 0.0f);

	// Angle�� ���� ���� �����Ѵ�.
	float spotIntensity = smoothstep(minCos, maxCos, cosAngle);
    lightStrength *= spotIntensity;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

/*
�ȼ� ������ ���Ͽ� �������� ����Ѵ�.
*/
float4 ComputeLighting(StructuredBuffer<Light> lights, Material mat, float3 pos, float3 normal, float3 toEye)
{
	float3 result = 0.0f;

	for (uint i = 0; i < LIGHT_NUM; ++i)
	{
		if (lights[i].enabled == 0)
			continue;

		switch (lights[i].type)
		{
		case DIRECTIONAL_LIGHT:
			result += ComputeDirectionalLight(lights[i], mat, normal, toEye);
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

/*
��� ����Ʈ �� ���⼺ ����Ʈ�� ����Ͽ� ����Ѵ�.
*/
float4 ComputeOnlyDirectionalLight(StructuredBuffer<Light> lights, Material mat, float3 normal, float3 toEye)
{
	float3 result = 0.0f;

	for (uint i = 0; i < LIGHT_NUM; ++i)
	{
		if (lights[i].enabled == 0)
			continue;

		switch (lights[i].type)
		{
		case DIRECTIONAL_LIGHT:
			result += ComputeDirectionalLight(lights[i], mat, normal, toEye);
			break;
		}
	}

	return float4(result, 0.0f);
}