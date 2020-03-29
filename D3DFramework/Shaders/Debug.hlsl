#include "Pass.hlsl"

struct VertexIn
{
	float3 posW : POSITION;
	float4 color: COLOR;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float4 color: COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	vout.posH = mul(float4(vin.posW, 1.0f), gViewProj);
	vout.color = vin.color;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.color;
}
