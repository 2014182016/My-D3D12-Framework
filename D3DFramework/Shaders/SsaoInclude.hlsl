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
	// z 성분에 대해 NDC 공간에서 시야 공간으로의 계산을
	// 역으로 수행할 수 있다. A = gProj[2, 2], B = gProj[3, 2]로
	// 두었을 때, z_ndc = A + B / viewZ이다.
	float viewZ = gProj[3][2] / (z_ndc - gProj[2][2]);
	return viewZ;
}

float OcclusionFunction(float distZ)
{
	// 만일 depth(q)가 depth(p)의 뒤에 있다면 q는 p를 가릴 수 없다.
	// 또한, depth(q)와 depth(p)가 충분히 가까울 때에도 q가 p를 가리지
	// 않는 것으로 판정한다. 왜냐하면 q가 p를 가리기 위해서는 q가 적어도
	// Epsilon만큼 p보다 앞에 있어야 하기 때문이다.

	float occlusion = 0.0f;
	if (distZ > gSurfaceEpsilon)
	{
		float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;

		//distZ가 gOcclusionFadeStart에서 gOcclusionFadeEnd로 증가함에
		// 따라 차폐도를 1에서 0으로 선형 감소한다.
		occlusion = saturate((gOcclusionFadeEnd - distZ) / fadeLength);
	}

	return occlusion;
}