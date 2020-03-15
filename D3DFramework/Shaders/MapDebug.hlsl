#include "Common.hlsl"

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

float4 ShadowMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gShadowMaps[0].Sample(gsamLinearWrap, pin.mTexC).rrr, 1.0f);
}

float4 DiffuseMapDebugPS(VertexOut pin) : SV_Target
{
	return gDiffuseMap.Sample(gsamLinearWrap, pin.mTexC);
}

float4 SpecularMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSpecularRoughnessMap.Sample(gsamLinearWrap, pin.mTexC).rgb, 1.0f);
}

float4 RoughnessMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSpecularRoughnessMap.Sample(gsamLinearWrap, pin.mTexC).aaa, 1.0f);
}

float4 NormalMapDebugPS(VertexOut pin) : SV_Target
{
	return gNormalMap.Sample(gsamLinearWrap, pin.mTexC);
}

float4 DepthMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gDepthMap.Sample(gsamLinearWrap, pin.mTexC).rrr, 1.0f);
}

float4 PositionMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gPositonMap.Sample(gsamLinearWrap, pin.mTexC).rgb, 1.0f);
}

float4 SsaoMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSsaoMap.Sample(gsamLinearWrap, pin.mTexC).rrr, 1.0f);
}

float4 SsrMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSsrMap.Sample(gsamLinearWrap, pin.mTexC).rgb, 1.0f);
}

float4 BluredSsrMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gBluredSsrMap.Sample(gsamLinearWrap, pin.mTexC).rgb, 1.0f);
}