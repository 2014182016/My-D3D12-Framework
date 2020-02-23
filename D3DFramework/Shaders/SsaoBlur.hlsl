//=============================================================================
// SsaoBlur.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// SSAO �ʿ� ���� ����� �����ڸ� ���� �帮�⸦ �����Ѵ�. ��� ��忡�� ������
// ������ ��ȯ�� ���ϱ� ���� ��� ���̴� ��� �ȼ� ���̴����� �帮�⸦ ����
// �Ѵ�. ������ ���� �޸𸮸� ����� �� �����Ƿ� �ؽ�ó�� ������ ĳ�÷� Ȱ���Ѵ�.
// SSAO ���� 16��Ʈ �ؽ�ó ������ ����ϹǷ� �� �ؼ��� ũ�Ⱑ �۴�. ���� ĳ�ÿ�
// ���� ���� �ؼ��� ���� �� �ִ�.
//=============================================================================

//#include "SsaoInclude.hlsl"
#include "Common.hlsl"

struct VertexOut
{
	float4 mPosH  : SV_POSITION;
	float2 mTexC  : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.mTexC = gTexCoords[vid];
	vout.mPosH = float4(2.0f * vout.mTexC.x - 1.0f, 1.0f - 2.0f * vout.mTexC.y, 0.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float3 centerNormal = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.mTexC, 0.0f).xyz);
	centerNormal = mul(centerNormal, (float3x3)gView);
	float centerDepth = gDepthMap.SampleLevel(gsamDepthMap, pin.mTexC, 0.0f).r;
	centerDepth = NdcDepthToViewDepth(centerDepth);

	// �帮�� �� ����ġ���� 1���� float �迭�� Ǯ�� ���´�.
	float blurWeights[12] =
	{
		gBlurWeights[0].x, gBlurWeights[0].y, gBlurWeights[0].z, gBlurWeights[0].w,
		gBlurWeights[1].x, gBlurWeights[1].y, gBlurWeights[1].z, gBlurWeights[1].w,
		gBlurWeights[2].x, gBlurWeights[2].y, gBlurWeights[2].z, gBlurWeights[2].w,
	};

	float2 texOffset;
	float4 color = 0.0f;
	if (gHorizontalBlur)
	{
		texOffset = float2(gInvRenderTargetSize.x, 0.0f);
		color = blurWeights[gBlurRadius] * gSsaoMap.SampleLevel(gsamPointClamp, pin.mTexC, 0.0);
	}
	else
	{
		texOffset = float2(0.0f, gInvRenderTargetSize.y);
		color = blurWeights[gBlurRadius] * gSsaoMap2.SampleLevel(gsamPointClamp, pin.mTexC, 0.0);
	}

	// ���� �� �߾��� ���� �׻� ���տ� �⿩�Ѵ�.
	float totalWeight = blurWeights[gBlurRadius];

	for (float i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		// �߾��� ���� �̹� �ջ��Ͽ���.
		if (i == 0)
			continue;

		float2 tex = pin.mTexC + i * texOffset;

		float3 neighborNormal = gNormalMap.SampleLevel(gsamPointClamp, tex, 0.0f).xyz;
		neighborNormal = mul(neighborNormal, (float3x3)gView);
		float  neighborDepth = gDepthMap.SampleLevel(gsamDepthMap, tex, 0.0f).r;
		neighborDepth = NdcDepthToViewDepth(neighborDepth);

		// �߾��� ���� �� �̿��� ���� ���̰� �ʹ� ũ��(�����̵� �����̵�)
		// ǥ���� �ҿ��� ��迡 ���� �ִ� ������ �����Ѵ�. �׷� ǥ������
		// �帮�⿡�� �����Ѵ�.

		if (dot(neighborNormal, centerNormal) >= 0.8f &&
			abs(neighborDepth - centerDepth) <= 0.2f)
		{
			float weight = blurWeights[i + gBlurRadius];

			// �̿� �ȼ����� �߰��Ѵ�(�׷��� ���� �ȼ��� �� �������).
			if (gHorizontalBlur)
			{
				color += weight * gSsaoMap.SampleLevel(gsamPointClamp, tex, 0.0);
			}
			else
			{
				color += weight * gSsaoMap2.SampleLevel(gsamPointClamp, tex, 0.0);
			}

			totalWeight += weight;
		}
	}

	// ��꿡�� ���ܵ� ǥ���� ���� �� �����Ƿ� ������ �����
	// ����ġ���� ������ �������ش�.
	return color / totalWeight;
}
