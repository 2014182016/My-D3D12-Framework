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

	// �� �𼭸��� ���� ����Ʈ���� ���� �����ϰ� gCorner�� ���� ����Ѵ�.
	if ((groupThreadID.x == 0 && groupThreadID.y == 0) ||
		(groupThreadID.x == 15 && groupThreadID.y == 0) ||
		(groupThreadID.x == 0 && groupThreadID.y == 15) ||
		(groupThreadID.x == 15 && groupThreadID.y == 15))
	{
		gGroupResults[groupIndex] = gHeightMap.Load(uint3(dispatchThreadID.xy, 0)).r;

		gCorners[groupThreadID.x / 15][groupThreadID.y / 15] =
			float3(groupThreadID.x / 15, gGroupResults[groupIndex], groupThreadID.y / 15);

		// ���� �������� �δ��� ������ �����Ѵ�.
		gCorners[groupThreadID.x / 15][groupThreadID.y / 15].x /= 64.0f;
		gCorners[groupThreadID.x / 15][groupThreadID.y / 15].z /= 64.0f;
	}

	// �׷��� ��� �����尡 �б� �۾��� ��ĥ ������ ������ �����Ѵ�.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 2 ///////////////////////////////////

	// �� �𼭸��� ����� ����Ѵ�.
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
	// �𼭸��� �ƴ� ��������� HeightMap�� ���� �����Ѵ�.
	else
	{
		gGroupResults[groupIndex] = gHeightMap.Load(uint3(dispatchThreadID.xy, 0)).r;
	}

	// �׷��� ��� �����尡 �б� �۾��� ��ĥ ������ ������ �����Ѵ�.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 3 ///////////////////////////////////

	// ������ �׷��� ù �����常 ����Ͽ� ��� ������ ����� ���Ѵ�.
	if (groupIndex == 0)
	{
		// �켱 ��� ���� ���͸� ���Ѵ�.
		float3 n = normalize(gRawNormals[0][0] + gRawNormals[0][1] + gRawNormals[1][0] + gRawNormals[1][1]);

		// �������� �������� ���� �������� �����Ѵ�.
		float3 p = float3(0.0f, 1e9f, 0.0f);
		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				if (gCorners[i][j].y < p.y)
					p = gCorners[i][j];
			}
		}

		// ���������� �� ���� ������ �̿��ؼ� ��� ����� �����Ѵ�.
		gPlane = CreatePlaneFromPointAndNormal(n, p);
	}

	// �׷��� ��� �����尡 �б� �۾��� ��ĥ ������ ������ �����Ѵ�.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 4 ///////////////////////////////////

	gGroupResults[groupIndex] = ComputeDistanceFromPlane(gPlane,
		float3(dispatchThreadID.x / 15.0f, gGroupResults[groupIndex], dispatchThreadID.y / 15.0f));

	// �׷��� ��� �����尡 �б� �۾��� ��ĥ ������ ������ �����Ѵ�.
	GroupMemoryBarrierWithGroupSync();

	//////////////////////////////////// Phase 5 ///////////////////////////////////

	if (groupIndex == 0)
	{
		// �� �����尡 �׷��� ù �������̸� �� Ÿ���� ǥ�������� ����Ѵ�.
		// ������ ���̵��� ����� �ƴ϶� ������ ���ϴ� ������ ������ ��.
		// ���� ������ ������ ���� ���̴� 0�̹Ƿ� �׳� ����� ���ϸ�
		// 0.0�� ���͹�����.

		float stddev = 0.0f;

		for (int i = 0; i < 16 * 16; ++i)
			stddev += pow(gGroupResults[i], 2);

		stddev /= ((16.0f * 16.0f) - 1.0f);
		stddev = sqrt(stddev);

		// ���� ���� ���Ϳ� ǥ�� ������ ��� ���ۿ� ����Ѵ�.
		// ���� ���� ���̴��� ���� ���̴��� �̵��� ����Ѵ�.
		gStdDevMap[uint2(groupID.x, groupID.y)] = float4(gPlane.xyz, stddev);
	}
}

float3 Sobel(uint2 uv)
{
	// �ʿ��� �����µ��� ����Ѵ�.
	uint2 o00 = uv + uint2(-1, -1);
	uint2 o10 = uv + uint2(0, -1);
	uint2 o20 = uv + uint2(1, -1);
	uint2 o01 = uv + uint2(-1, 0);
	uint2 o02 = uv + uint2(-1, 1);
	uint2 o12 = uv + uint2(0, 1);
	uint2 o21 = uv + uint2(1, 0);
	uint2 o22 = uv + uint2(1, 1);

	// �Һ� ���͸� �����Ϸ��� ���� �ʼ� ������ ���� �ȼ��� �ʿ��Ѵ�.
	float h00 = gHeightMap.Load(uint3(o00, 0)).r;
	float h10 = gHeightMap.Load(uint3(o10, 0)).r;
	float h20 = gHeightMap.Load(uint3(o20, 0)).r;
	float h01 = gHeightMap.Load(uint3(o01, 0)).r;
	float h02 = gHeightMap.Load(uint3(o02, 0)).r;
	float h12 = gHeightMap.Load(uint3(o12, 0)).r;
	float h21 = gHeightMap.Load(uint3(o21, 0)).r;
	float h22 = gHeightMap.Load(uint3(o22, 0)).r;

	// �Һ� ���͵��� ���Ѵ�.
	float gx = h00 - h20 + 2.0f * h01 - 2.0f * h21 + h02 - h22;
	float gy = h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;

	// ���� z ������ �����Ѵ�.
	float gz = 0.01f * sqrt(max(0.0f, 1.0f - gx * gx - gy * gy));

	return normalize(float3(2.0f * gx, gz, 2.0f * gy));
}

[numthreads(16, 16, 1)]
void CS_Normal(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	gNormalMap[dispatchThreadID.xy] = float4(Sobel(dispatchThreadID.xy), 1.0f);
}