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

	// �ش� �ȼ��� ��ϵǾ� �ִ��� Ȯ���Ѵ�.
	if (diffuse.a <= 0.0f)
		return 0.0f;

	// ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - position.xyz;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse�� ���������� �����ִ� Ambient��
	float4 ambient = gAmbientLight * diffuse;

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuse, specular, shininess };

	// Lighting�� �ǽ��Ѵ�.
	float4 directLight = ComputeShadowLighting(gLights, mat, position.xyz, normal, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	// SSAO�� ����ϴ� ��� Ssao�ʿ��� ���󵵸� �����ͼ� 
	// ���� ���⸦ ���δ�.
	float4 ssaoPosH = mul(float4(position.xyz, 1.0f), gViewProjTex);
	ssaoPosH /= ssaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy, 0.0f).r;
	litColor *= ambientAccess;
#endif

	// Diferred rendering������ �������� ��ü�� ������ �����ϴ�.
	litColor.a = 1.0f;

	if (gFogEnabled)
	{
		// �Ȱ��� ����Ѵٸ� �Ȱ��� ������ ���� ���� ���´�.
		litColor = FogBlend(litColor, distToEye);
	}

	return litColor;
}