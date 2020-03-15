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

	// ��ũ�� ��ǥ�迡�� ���� ��ǥ��� ��ȯ�Ѵ�.
	vout.mPosH = float4(2.0f * vout.mTexC.x - 1.0f, 1.0f - 2.0f * vout.mTexC.y, 0.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuse = 0.0f; float4 position = 0.0f;
	float3 specular = 0.0f; float3 normal = 0.0f;
	float roughness = 0.0f; float depth = 0.0f;
	GetGBufferAttribute(int3(pin.mPosH.xy, 0), diffuse, specular, roughness, position, normal, depth);

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
	litColor.rgb *= GetAmbientAccess(position.xyz);
#endif

	// Diferred rendering������ �������� ��ü�� ������ �����ϴ�.
	litColor.a = 1.0f;

	if (gFogEnabled)
	{
		litColor = GetFogBlend(litColor, distToEye);
	}

	return litColor;
}