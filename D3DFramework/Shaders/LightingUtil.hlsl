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

// 점광과 점적광에 적용하는 선형 감쇠 계수를 계산한다.
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// 프레넬 방정식의 슐릭 근사를 구한다.
// 즉, 번섭이 n인 표면에서 프레넬 효과에 의해 반사되는 빛의 비율을
// 빛 벡터 L과 표면 법선 n 사이의 각도에 근거해서 근사한다.
float3 SchlickFresnel(float3 r0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = r0 + (1.0f - r0)*(pow(f0, 5));

    return reflectPercent;
}

// 눈에 도달한 반사광의 양을 계산한다.
// 즉, 분산 반사와 반영 반사의 합을 구한다.
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.mShininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 specularFactor = SchlickFresnel(mat.mSpecular, halfVec, lightVec);

    float3 specAlbedo = specularFactor * roughnessFactor;

    // 반영 반사율 공식이 [0,1] 구간 바깥의 값을 낼 수도 있지만,
	// 우리는 LDR 렌더링을 구현하므로, 반사율을 1미만으로 낮춘다.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.mDiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material mat, float3 normal, float3 toEye)
{
    // 빛 벡터는 광선들이 나아가는 방향의 반대 방향을 가리킨다.
    float3 lightVec = -light.mDirection;

    // 람베트르 코사인 법칙에 따라 빛의 세기를 줄인다.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.mStrength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // 표면에서 광원으로의 벡터
    float3 lightVec = light.mPosition - pos;

    // 광원과 표면 사이의 거리
    float d = length(lightVec);

    //범위 판정
    if(d > light.mFalloffEnd)
		return float3(0.0f, 0.0f, 0.0f);

    // 빛 벡터를 정규화한다.
    lightVec /= d;

    // 람베트르 코사인 법칙에 따라 빛의 세기를 줄인다.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.mStrength * ndotl;

    // 거리에 따라 빛을 감쇠한다.
    float att = CalcAttenuation(d, light.mFalloffStart, light.mFalloffEnd);
    lightStrength = lightStrength * att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // 표면에서 광원으로의 벡터
    float3 lightVec = light.mPosition - pos;

    // 광원과 표면 사이의 거리
    float d = length(lightVec);

    // 범위 판정
    if(d > light.mFalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    // 빛 벡터를 정규화한다.
    lightVec /= d;

    // 람베트르 코사인 법칙에 따라 빛의 세기를 줄인다.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.mStrength * ndotl;

    // 거리에 따라 빛을 감쇠한다.
    float att = CalcAttenuation(d, light.mFalloffStart, light.mFalloffEnd);
    lightStrength *= att;

    // 점적광 계수로 비례한다.
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