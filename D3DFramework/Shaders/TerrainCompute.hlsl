Texture2D<float4> gHeightMap : register(t0);
RWTexture2D<float4> gStdDevMap : register(u0);
RWTexture2D<float4> gNormalMap : register(u1);

float4 CreatePlaneFromPointAndNormal(float3 n, float3 p)
{
	return float4(n, -(n.x * p.x + n.y * p.y + n.z * p.z));
}

float ComputeDistanceFromPlane(float4 plane, float3 position)
{
	return dot(plane.xyz, position) - plane.w;
}

groupshared float gGroupResults[16 * 16];
groupshared float3 gRawNormals[2][2];
groupshared float3 gCorners[2][2];
groupshared float4 gPlane;

[numthreads(16, 16, 1)]
void CS_StdDev(uint3 groupID : SV_GroupID, uint3 dispatchThreadID : SV_DispatchThreadID,
	uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	//////////////////////////////////// Phase 1 ///////////////////////////////////

	// 각 모서리에 대해 하이트맵의 값을 저장하고 gCorner의 값을 계산한다.
	if ((groupThreadID.x == 0 && groupThreadID.y == 0) ||
		(groupThreadID.x == 15 && groupThreadID.y == 0) ||
		(groupThreadID.x == 0 && groupThreadID.y == 15) ||
		(groupThreadID.x == 15 && groupThreadID.y == 15))
	{
		gGroupResults[groupIndex] = gHeightMap.Load(uint3(dispatchThreadID.xy, 0)).r;

		gCorners[groupThreadID.x / 15][groupThreadID.y / 15] =
			float3(groupThreadID.x / 15, gGroupResults[groupIndex], groupThreadID.y / 15);

		// 높이 범위들의 부당한 편향을 보정한다.
		gCorners[groupThreadID.x / 15][groupThreadID.y / 15].x /= 64.0f;
		gCorners[groupThreadID.x / 15][groupThreadID.y / 15].z /= 64.0f;
	}

	// 그룹의 모든 스레드가 읽기 작업을 마칠 때까지 실행을 차단한다.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 2 ///////////////////////////////////

	// 각 모서리는 노멀을 계산한다.
	if (groupThreadID.x == 0 && groupThreadID.y == 0)
	{
		gRawNormals[0][0] = normalize(cross(gCorners[0][1] - gCorners[0][0], gCorners[1][0] - gCorners[0][0]));
	}
	else if (groupThreadID.x == 15 && groupThreadID.y == 0)
	{
		gRawNormals[1][0] = normalize(cross(gCorners[0][0] - gCorners[1][0], gCorners[1][1] - gCorners[1][0]));
	}
	else if (groupThreadID.x == 0 && groupThreadID.y == 15)
	{
		gRawNormals[0][1] = normalize(cross(gCorners[1][1] - gCorners[0][1], gCorners[0][0] - gCorners[0][1]));
	}
	else if (groupThreadID.x == 15 && groupThreadID.y == 15)
	{
		gRawNormals[1][1] = normalize(cross(gCorners[1][0] - gCorners[1][1], gCorners[0][1] - gCorners[1][1]));
	}
	// 모서리가 아닌 스레드들은 HeightMap의 값을 적재한다.
	else
	{
		gGroupResults[groupIndex] = gHeightMap.Load(uint3(dispatchThreadID.xy, 0)).r;
	}

	// 그룹의 모든 스레드가 읽기 작업을 마칠 때까지 실행을 차단한다.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 3 ///////////////////////////////////

	// 스레드 그룹의 첫 스레드만 사용하여 평면 방정식 계수를 구한다.
	if (groupIndex == 0)
	{
		// 우선 평면 법선 벡터를 구한다.
		float3 n = normalize(gRawNormals[0][0] + gRawNormals[0][1] + gRawNormals[1][0] + gRawNormals[1][1]);

		// 다음으로 기준으로 삼을 최저점을 결정한다.
		float3 p = float3(0.0f, 1e9f, 0.0f);
		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				if (gCorners[i][j].y < p.y)
					p = gCorners[i][j];
			}
		}

		// 마지막으로 그 점과 법선을 이용해서 평면 계수를 결정한다.
		gPlane = CreatePlaneFromPointAndNormal(n, p);
	}

	// 그룹의 모든 스레드가 읽기 작업을 마칠 때까지 실행을 차단한다.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 4 ///////////////////////////////////

	gGroupResults[groupIndex] = ComputeDistanceFromPlane(gPlane,
		float3(dispatchThreadID.x / 15.0f, gGroupResults[groupIndex], dispatchThreadID.y / 15.0f));

	// 그룹의 모든 스레드가 읽기 작업을 마칠 때까지 실행을 차단한다.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 5 ///////////////////////////////////

	if (groupIndex == 0)
	{
		// 이 스레드가 그룹의 첫 스레드이면 이 타일의 표준편차를 계산한다.
		// 평면과의 차이들의 평균이 아니라 편차를 구하는 것임을 주의할 것.
		// 이제 평면상의 임의의 점의 높이는 0이므로 그냥 평균을 구하면
		// 0.0이 나와버린다.

		float stddev = 0.0f;

		for (int i = 0; i < 16 * 16; ++i)
			stddev += pow(gGroupResults[i], 2);

		stddev /= ((16.0f * 16.0f) - 1.0f);
		stddev = sqrt(stddev);

		// 이제 법선 벡터와 표준 편차를 출력 버퍼에 기록한다.
		// 이후 영역 셰이더와 덮개 셰이더가 이들을 사용한다.
		gStdDevMap[uint2(groupID.x, groupID.y)] = float4(gPlane.xyz, stddev);
	}
}

float3 Sobel(uint2 uv)
{
	// 필요한 오프셋들을 계산한다.
	uint2 o00 = uv + uint2(-1, -1);
	uint2 o10 = uv + uint2(0, -1);
	uint2 o20 = uv + uint2(1, -1);
	uint2 o01 = uv + uint2(-1, 0);
	uint2 o02 = uv + uint2(-1, 1);
	uint2 o12 = uv + uint2(0, 1);
	uint2 o21 = uv + uint2(1, 0);
	uint2 o22 = uv + uint2(1, 1);

	// 소벨 필터를 적용하려면 현재 필셀 주위의 여덟 픽셀이 필요한다.
	float h00 = gHeightMap.Load(uint3(o00, 0)).r;
	float h10 = gHeightMap.Load(uint3(o10, 0)).r;
	float h20 = gHeightMap.Load(uint3(o20, 0)).r;
	float h01 = gHeightMap.Load(uint3(o01, 0)).r;
	float h02 = gHeightMap.Load(uint3(o02, 0)).r;
	float h12 = gHeightMap.Load(uint3(o12, 0)).r;
	float h21 = gHeightMap.Load(uint3(o21, 0)).r;
	float h22 = gHeightMap.Load(uint3(o22, 0)).r;

	// 소벨 필터들을 평가한다.
	float gx = h00 - h20 + 2.0f * h01 - 2.0f * h21 + h02 - h22;
	float gy = h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;

	// 빠진 z 성분을 생성한다.
	float gz = 0.01f * sqrt(max(0.0f, 1.0f - gx * gx - gy * gy));

	return normalize(float3(2.0f * gx, gz, 2.0f * gy));
}

[numthreads(16, 16, 1)]
void CS_Normal(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	gNormalMap[dispatchThreadID.xy] = float4(Sobel(dispatchThreadID.xy), 1.0f);
}