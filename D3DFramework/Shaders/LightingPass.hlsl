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
	float4 diffuse;
	float3 specular, posW, normal;
	float roughness, depth;
	GetGBufferAttribute(int3(pin.mPosH.xy, 0), diffuse, specular, roughness, posW, normal, depth);

	// ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - posW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse�� ���������� �����ִ� Ambient��
	float4 ambient = gAmbientLight * diffuse;

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuse, specular, shininess };

	// Lighting�� �ǽ��Ѵ�.
	float4 directLight = ComputeShadowLighting(gLights, mat, posW, normal, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	litColor *= GetAmbientAccess(posW);
#endif

	// �л� �������� ���ĸ� �����´�.
	litColor.a = diffuse.a;

	if (gFogEnabled)
	{
		litColor = GetFogBlend(litColor, distToEye);
	}

	return litColor;
}