//***************************************************************************************
// ShadowMapDebug.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 mPosL    : POSITION;
	float2 mTexC    : TEXCOORD;
};

struct VertexOut
{
	float4 mPosH    : SV_POSITION;
	float2 mTexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	vout.mPosH = float4(vin.mPosL, 1.0f);
	vout.mTexC = vin.mTexC;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return float4(gShadowMap[0].Sample(gsamLinearWrap, pin.mTexC).rrr, 1.0f);
}


