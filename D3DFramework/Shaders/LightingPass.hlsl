#include "Common.hlsl"

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.mTexC = gTexCoords[vid];

	// 스크린 좌표계에서 동차 좌표계로 변환한다.
	vout.mPosH = float4(2.0f * vout.mTexC.x - 1.0f, 1.0f - 2.0f * vout.mTexC.y, 0.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuse;
	float3 specular, posW, normal;
	float roughness, depth;
	GetGBufferAttribute(int3(pin.mPosH.xy, 0), diffuse, specular, roughness, posW, normal, depth);

	// 조명되는 픽셀에서 눈으로의 벡터
	float3 toEyeW = gEyePosW - posW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse를 전반적으로 밝혀주는 Ambient항
	float4 ambient = gAmbientLight * diffuse;

	// roughness와 normal를 이용하여 shininess를 계산한다.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuse, specular, shininess };

	// Lighting을 실시한다.
	float4 directLight = ComputeShadowLighting(gLights, mat, posW, normal, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	litColor *= GetAmbientAccess(posW);
#endif

	// 분산 재질에서 알파를 가져온다.
	litColor.a = diffuse.a;

	if (gFogEnabled)
	{
		litColor = GetFogBlend(litColor, distToEye);
	}

	return litColor;
}