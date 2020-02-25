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

	// ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - posW.xyz;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse�� ���������� �����ִ� Ambient��
	float4 ambient = gAmbientLight * diffuseAlbedo;

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuseAlbedo, specular, shininess };

	// Lighting�� �ǽ��Ѵ�.
	float4 directLight = ComputeShadowLighting(gLights, mat, posW.xyz, normal, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	float4 ssaoPosH = mul(posW, gViewProjTex);
	ssaoPosH /= ssaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy, 0.0f).r;
	litColor *= ambientAccess;
#endif

	// �л� �������� ���ĸ� �����´�.
	litColor.a = diffuseAlbedo.a;

	if (gFogEnabled)
	{
		float fogAmount;

		switch (gFogType)
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