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
	float4 diffuse = 0.0f; float4 position = 0.0f;
	float3 specular = 0.0f; float3 normal = 0.0f;
	float roughness = 0.0f; float depth = 0.0f;
	GetGBufferAttribute(int3(pin.mPosH.xy, 0), diffuse, specular, roughness, position, normal, depth);

	// 해당 픽셀이 기록되어 있는지 확인한다.
	if (diffuse.a <= 0.0f)
		return 0.0f;

	// 조명되는 픽셀에서 눈으로의 벡터
	float3 toEyeW = gEyePosW - position.xyz;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse를 전반적으로 밝혀주는 Ambient항
	float4 ambient = gAmbientLight * diffuse;

	// roughness와 normal를 이용하여 shininess를 계산한다.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuse, specular, shininess };

	// Lighting을 실시한다.
	float4 directLight = ComputeShadowLighting(gLights, mat, position.xyz, normal, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	litColor.rgb *= GetAmbientAccess(position.xyz);
#endif

	// Diferred rendering에서는 불투명한 물체만 렌더링 가능하다.
	litColor.a = 1.0f;

	if (gFogEnabled)
	{
		litColor = GetFogBlend(litColor, distToEye);
	}

	return litColor;
}