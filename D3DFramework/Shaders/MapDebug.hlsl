#include "Common.hlsl"

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

float4 ShadowMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gShadowMaps[0].Sample(gsamAnisotropicWrap, pin.mTexC).rrr, 1.0f);
}

float4 DiffuseMapDebugPS(VertexOut pin) : SV_Target
{
	return gDiffuseMap.Sample(gsamAnisotropicWrap, pin.mTexC);
}

float4 SpecularMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSpecularAndRoughnessMap.Sample(gsamAnisotropicWrap, pin.mTexC).rgb, 1.0f);
}

float4 NormalMapDebugPS(VertexOut pin) : SV_Target
{
	return gNormalMap.Sample(gsamAnisotropicWrap, pin.mTexC);
}

float4 RoughnessMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSpecularAndRoughnessMap.Sample(gsamAnisotropicWrap, pin.mTexC).aaa, 1.0f);
}

float4 DepthMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gDepthMap.Sample(gsamAnisotropicWrap, pin.mTexC).rrr, 1.0f);
}

float4 PositionMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gPositonMap.Sample(gsamAnisotropicWrap, pin.mTexC).rgb, 1.0f);
}