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

	// ���� ���� ��ġ�� ť��� ��ȸ ���ͷ� ����Ѵ�.
	vout.PosL = vin.PosL;

	float4 posW = mul(float4(vin.PosL, 1.0f), gObjWorld);

	// z / w = 1�� �ǵ���(��, �ϴ� ���� �׻� �� ��鿡 �ֵ���) z = w�� �����Ѵ�.
	vout.PosH = mul(posW, gViewProj).xyww;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	if (gCurrentSkyCubeMapIndex == DISABLED)
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	return gCubeMaps[gCurrentSkyCubeMapIndex].Sample(gsamLinearWrap, pin.PosL);
}

