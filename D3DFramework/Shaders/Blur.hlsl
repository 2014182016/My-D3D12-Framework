//=============================================================================
// Performs a separable Guassian blur with a blur radius up to 5 pixels.
//=============================================================================

static const int gMaxBlurRadius = 5;

cbuffer cbSettings : register(b0)
{
	int gBlurRadius;

	float w0;
	float w1;
	float w2;
	float w3;
	float w4;
	float w5;
	float w6;
	float w7;
	float w8;
	float w9;
	float w10;
};

Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N + 2 * gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(N, 1, 1)]
void CS_BlurHorz(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
	// 가우스 가중치를 나열한다.
	float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	// 화면의 픽셀 범위를 벗어나는 값에 대해 정의한다.
	if (groupThreadID.x < gBlurRadius)
	{
		int x = max(dispatchThreadID.x - gBlurRadius, 0);
		gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
	}
	if (groupThreadID.x >= N - gBlurRadius)
	{
		int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
		gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
	}

	gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

	// 그룹 스레드가 여기에 도달할 때까지 기다린다.
	GroupMemoryBarrierWithGroupSync();

	float4 blurColor = 0.0f;

	for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		// 가중치를 사용하여 수평 부분 픽셀을 섞는다.
		int k = groupThreadID.x + gBlurRadius + i;
		blurColor += weights[i + gBlurRadius] * gCache[k];
	}

	// 결과값을 저장한다.
	gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void CS_BlurVert(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
	// 가우스 가중치를 나열한다.
	float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	// 화면의 픽셀 범위를 벗어나는 값에 대해 정의한다.
	if (groupThreadID.y < gBlurRadius)
	{
		int y = max(dispatchThreadID.y - gBlurRadius, 0);
		gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
	}
	if (groupThreadID.y >= N - gBlurRadius)
	{
		int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y - 1);
		gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
	}

	gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

	// 그룹 스레드가 여기에 도달할 때까지 기다린다.
	GroupMemoryBarrierWithGroupSync();

	float4 blurColor = float4(0, 0, 0, 0);

	for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		// 가중치를 사용하여 수직 부분 픽셀을 섞는다.
		int k = groupThreadID.y + gBlurRadius + i;
		blurColor += weights[i + gBlurRadius] * gCache[k];
	}

	// 결과값을 저장한다.
	gOutput[dispatchThreadID.xy] = blurColor;
}