#include "Common.hlsl"

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

float4 PS(VertexOut pin) : SV_Target
{
	int3 texcoord = int3(pin.mPosH.xy, 0);
	float4 diffuseAlbedo = gDiffuseMap.Load(texcoord);
	float4 specularAndroughness = gSpecularAndRoughnessMap.Load(texcoord);
	float3 specular = specularAndroughness.rgb;
	float roughness = specularAndroughness.a;
	float3 normal = gNormalMap.Load(texcoord).rgb;
	float depth = gDepthMap.Load(texcoord).r;
	float4 posW = gPositonMap.Load(texcoord);

	// 조명되는 픽셀에서 눈으로의 벡터
	float3 toEyeW = gEyePosW - posW.xyz;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse를 전반적으로 밝혀주는 Ambient항
	float4 ambient = gAmbientLight * diffuseAlbedo;

	// roughness와 normal를 이용하여 shininess를 계산한다.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuseAlbedo, specular, shininess };

	// Lighting을 실시한다.
	float4 directLight = ComputeShadowLighting(gLights, mat, posW.xyz, normal, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	float4 ssaoPosH = mul(posW, gViewProjTex);
	ssaoPosH /= ssaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy, 0.0f).r;
	litColor *= ambientAccess;
#endif

	// 분산 재질에서 알파를 가져온다.
	litColor.a = diffuseAlbedo.a;

	if (gFogEnabled)
	{
		float fogAmount;

		switch (gFogType)
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