//=============================================================================
// SsaoBlur.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// SSAO 맵에 대해 양방향 가장자리 보존 흐리기를 수행한다. 계산 모드에서 렌더링
// 모드로의 전환을 피하기 위해 계산 셰이더 대신 픽셀 셰이더에서 흐리기를 수행
// 한다. 이제는 공유 메모리를 사용할 수 없으므로 텍스처를 일종의 캐시로 활용한다.
// SSAO 맵은 16비트 텍스처 형식을 사용하므로 한 텍셀의 크기가 작다. 따라서 캐시에
// 많은 수의 텍셀을 담을 수 있다.
//=============================================================================

#include "Ssao.hlsl"
#include "Fullscreen.hlsl"

Texture2D gInputMap : register(t2);

float4 PS(VertexOut pin) : SV_Target
{
	// 월드 좌표계의 깊이를 뷰 공간으로 변환한다.
	float centerDepth = gDepthMap.SampleLevel(gsamDepthMap, pin.texC, 0.0f).r;
	centerDepth = NdcDepthToViewDepth(centerDepth);

	// 흐리기 핵 가중치들을 1차원 float 배열에 풀어 놓는다.
	float blurWeights[12] =
	{
		gBlurWeights[0].x, gBlurWeights[0].y, gBlurWeights[0].z, gBlurWeights[0].w,
		gBlurWeights[1].x, gBlurWeights[1].y, gBlurWeights[1].z, gBlurWeights[1].w,
		gBlurWeights[2].x, gBlurWeights[2].y, gBlurWeights[2].z, gBlurWeights[2].w,
	};

	float2 texOffset;
	if (gHorizontalBlur)
	{
		texOffset = float2(gSsaoRenderTargetInvSize.x, 0.0f);
	}
	else
	{
		texOffset = float2(0.0f, gSsaoRenderTargetInvSize.y);
	}

	// 필터 핵 중앙의 값은 항상 총합에 기여한다.
	float totalWeight = blurWeights[gBlurRadius];
	float4 color = blurWeights[gBlurRadius] * gInputMap.SampleLevel(gsamLinearWrap, pin.texC, 0.0);

	for (float i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		// 중앙의 값은 이미 합산하였다.
		if (i == 0)
			continue;

		float2 tex = pin.texC + i * texOffset;

		float  neighborDepth = gDepthMap.SampleLevel(gsamDepthMap, tex, 0.0f).r;
		neighborDepth = NdcDepthToViewDepth(neighborDepth);

		// 중앙의 값과 그 이웃의 값의 차이가 너무 크면 표본이 
		// 불연속 경계에 걸쳐 있는 것으로 간주한다. 그런 표본들은
		// 흐리기에서 제외한다.
		if (abs(neighborDepth - centerDepth) <= 0.2f)
		{
			float weight = blurWeights[i + gBlurRadius];

			// 이웃 픽셀들을 추가한다(그러면 현재 픽셀이 더 흐려진다).
			color += weight * gInputMap.SampleLevel(gsamLinearWrap, tex, 0.0);

			totalWeight += weight;
		}
	}

	// 계산에서 제외된 표본이 있을 수 있으므로 실제로 적용된
	// 가중치들의 합으로 나누어준다.
	return color / totalWeight;
}
