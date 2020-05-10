#include "Fullscreen.hlsl"

SamplerState gsamLinearClamp : register(s0);

Texture2D gColorMap : register(t0);
Texture2D gSpecularRoughnessMap : register(t1);
Texture2D gPositionMap : register(t2);
Texture2D gSsrMap : register(t3);
Texture2D gBluredSsrMap : register(t4);

float4 PS(VertexOut pin) : SV_Target
{
	uint3 texcoord = uint3(pin.posH.xy, 0);

	// 현재 후면 버퍼의 색상을 가져온다.
	float4 currentColor = gColorMap.Load(texcoord);
	if (gPositionMap.Load(texcoord).a <= 0.0f)
		return currentColor;

	// 각 픽셀에 적용할 속성을 가져온다.
	float roughness = gSpecularRoughnessMap.Load(texcoord).a;
	float shininess = 1.0f - roughness;
	float4 bluredSsr = gBluredSsrMap.Sample(gsamLinearClamp, pin.texC, 0.0f);

	// shininess가 높을수록 반사된 이미지가 보이고,
	// 낮을수록 원래 색상이 보인다.
	return lerp(currentColor, bluredSsr, bluredSsr.a * shininess);
}