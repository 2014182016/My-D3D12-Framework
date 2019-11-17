//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 mPosL : POSITION;
	float4 mColor: COLOR;
};

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float3 mPosW : POSITION;
	float4 mColor: COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	float4 posW = mul(float4(vin.mPosL, 1.0f), gBoundingWorld);
	vout.mPosW = posW.xyz;

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.mPosH = mul(posW, gViewProj);

	vout.mColor = vin.mColor;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.mColor;
}
