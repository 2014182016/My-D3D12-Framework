#include "Common.hlsl"

struct VertexIn
{
	float3 mPos : POSITION;
	float4 mColor: COLOR;
};

struct VertexOut
{
	float4 mPos : SV_POSITION;
	float4 mColor: COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	float4 posW = mul(float4(vin.mPos, 1.0f), gIdentity);
	vout.mPos = mul(posW, gViewProj);
	vout.mColor = vin.mColor;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.mColor;
}
