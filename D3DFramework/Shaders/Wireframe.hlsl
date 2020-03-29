#include "Common.hlsl"

struct VertexIn
{
	float3 posL : POSITION;
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

	float4 posW = mul(float4(vin.posL, 1.0f), gObjWorld);
	vout.posH = mul(posW, gViewProj);

	vout.color = vin.color;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.color;
}
