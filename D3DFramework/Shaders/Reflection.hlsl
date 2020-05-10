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

	// ���� �ĸ� ������ ������ �����´�.
	float4 currentColor = gColorMap.Load(texcoord);
	if (gPositionMap.Load(texcoord).a <= 0.0f)
		return currentColor;

	// �� �ȼ��� ������ �Ӽ��� �����´�.
	float roughness = gSpecularRoughnessMap.Load(texcoord).a;
	float shininess = 1.0f - roughness;
	float4 bluredSsr = gBluredSsrMap.Sample(gsamLinearClamp, pin.texC, 0.0f);

	// shininess�� �������� �ݻ�� �̹����� ���̰�,
	// �������� ���� ������ ���δ�.
	return lerp(currentColor, bluredSsr, bluredSsr.a * shininess);
}