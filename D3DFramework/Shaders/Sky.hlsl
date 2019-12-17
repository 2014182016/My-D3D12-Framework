#include "Common.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// 로컬 공간 위치를 큐브맵 조회 벡터로 사용한다.
	vout.PosL = vin.PosL;

	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// z / w = 1이 되도록(즉, 하늘 구가 항상 먼 평면에 있도록) z = w로 설정한다.
	vout.PosH = mul(posW, gViewProj).xyww;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	if (gCurrentSkyCubeMapIndex == -1)
		return gAmbientLight;
	return gCubeMaps[gCurrentSkyCubeMapIndex].Sample(gsamLinearWrap, pin.PosL);
}

