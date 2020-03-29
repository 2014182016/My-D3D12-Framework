#include "Common.hlsl"
#include "Fullscreen.hlsl"

float4 PS(VertexOut pin) : SV_Target
{
	uint3 texCoord = uint3(pin.posH.xy, 0);

	float4 diffuse = gDiffuseMap.Load(texCoord);
	float4 specularAndroughness = gSpecularRoughnessMap.Load(texCoord);
	float3 specular = specularAndroughness.rgb;
	float roughness = specularAndroughness.a;
	float4 position = gPositonMap.Load(texCoord);
	float3 normal = gNormalMap.Load(texCoord).xyz;
	float depth = gDepthMap.Load(texCoord).r;

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
	// SSAO를 사용하는 경우 Ssao맵에서 차폐도를 가져와서 
	// 빛의 세기를 줄인다.
	float4 ssaoPosH = mul(float4(position.xyz, 1.0f), gViewProjTex);
	ssaoPosH /= ssaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy, 0.0f).r;
	litColor *= ambientAccess;
#endif

	// Diferred rendering에서는 불투명한 물체만 렌더링 가능하다.
	litColor.a = 1.0f;

	if (gFogEnabled)
	{
		// 안개를 사용한다면 안개의 설정에 따라 색을 섞는다.
		litColor = FogBlend(litColor, distToEye);
	}

	return litColor;
}