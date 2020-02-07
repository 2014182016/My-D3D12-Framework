//=============================================================================
// Ssao.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//=============================================================================

cbuffer cbSsao : register(b0)
{
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gProjTex;
	float4 gOffsetVectors[14];

	float4 gBlurWeights[3];

	float2 gInvRenderTargetSize;

	float gOcclusionRadius;
	float gOcclusionFadeStart;
	float gOcclusionFadeEnd;
	float gSurfaceEpsilon;
};

cbuffer cbRootConstants : register(b1)
{
	bool gHorizontalBlur;
};

Texture2D gNormalMap    : register(t0);
Texture2D gDepthMap     : register(t1);
Texture2D gRandomVecMap : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

static const int gSampleCount = 14;
static const int gBlurRadius = 5;

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

float NdcDepthToViewDepth(float z_ndc)
{
	// z ���п� ���� NDC �������� �þ� ���������� �����
	// ������ ������ �� �ִ�. A = gProj[2, 2], B = gProj[3, 2]��
	// �ξ��� ��, z_ndc = A + B / viewZ�̴�.
	float viewZ = gProj[3][2] / (z_ndc - gProj[2][2]);
	return viewZ;
}

float OcclusionFunction(float distZ)
{
	// ���� depth(q)�� depth(p)�� �ڿ� �ִٸ� q�� p�� ���� �� ����.
	// ����, depth(q)�� depth(p)�� ����� ����� ������ q�� p�� ������
	// �ʴ� ������ �����Ѵ�. �ֳ��ϸ� q�� p�� ������ ���ؼ��� q�� ���
	// Epsilon��ŭ p���� �տ� �־�� �ϱ� �����̴�.

	float occlusion = 0.0f;
	if (distZ > gSurfaceEpsilon)
	{
		float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;

		//distZ�� gOcclusionFadeStart���� gOcclusionFadeEnd�� �����Կ�
		// ���� ���󵵸� 1���� 0���� ���� �����Ѵ�.
		occlusion = saturate((gOcclusionFadeEnd - distZ) / fadeLength);
	}

	return occlusion;
}