#include "Common.hlsl"

struct VertexIn
{
	float3 mPosL : POSITION;
};

struct VertexOut
{
	float4 mPosH : SV_POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	float4 posW = mul(float4(vin.mPosL, 1.0f), gDebugWorld);
	vout.mPosH = mul(posW, gViewProj);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return gDebugColor;
}
