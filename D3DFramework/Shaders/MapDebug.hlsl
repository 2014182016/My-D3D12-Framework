#include "Common.hlsl"

/*버텍스 쉐이더는 위젯의 버텍스 쉐이더를 사용한다.*/

struct VertexOut
{
	float4 posH : SV_POSITION;
	float2 texC : TEXCOORD;
};

float4 ShadowMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gShadowMaps[0].Sample(gsamLinearWrap, pin.texC).rrr, 1.0f);
}

float4 DiffuseMapDebugPS(VertexOut pin) : SV_Target
{
	return gDiffuseMap.Sample(gsamLinearWrap, pin.texC);
}

float4 SpecularMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSpecularRoughnessMap.Sample(gsamLinearWrap, pin.texC).rgb, 1.0f);
}

float4 RoughnessMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSpecularRoughnessMap.Sample(gsamLinearWrap, pin.texC).aaa, 1.0f);
}

float4 NormalMapDebugPS(VertexOut pin) : SV_Target
{
	return gNormalMap.Sample(gsamLinearWrap, pin.texC);
}

float4 DepthMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gDepthMap.Sample(gsamLinearWrap, pin.texC).rrr, 1.0f);
}

float4 PositionMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gPositonMap.Sample(gsamLinearWrap, pin.texC).rgb, 1.0f);
}

float4 SsaoMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSsaoMap.Sample(gsamLinearWrap, pin.texC).rrr, 1.0f);
}

float4 SsrMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gSsrMap.Sample(gsamLinearWrap, pin.texC).rgb, 1.0f);
}

float4 BluredSsrMapDebugPS(VertexOut pin) : SV_Target
{
	return float4(gBluredSsrMap.Sample(gsamLinearWrap, pin.texC).rgb, 1.0f);
}