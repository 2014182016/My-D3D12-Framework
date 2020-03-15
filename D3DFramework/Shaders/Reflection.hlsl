static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
}; 

Texture2D gColorMap : register(t0);
Texture2D gSpecularRoughnessMap : register(t1);
Texture2D gPositionMap : register(t2);
Texture2D gSsrMap : register(t3);
Texture2D gBluredSsrMap : register(t4);

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.mTexC = gTexCoords[vid];

	// 스크린 좌표계에서 동차 좌표계로 변환한다.
	vout.mPosH = float4(2.0f * vout.mTexC.x - 1.0f, 1.0f - 2.0f * vout.mTexC.y, 0.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	uint2 texcoord = pin.mPosH.xy;

	float4 currentColor = gColorMap.Load(uint3(texcoord, 0));
	if (gPositionMap.Load(uint3(texcoord, 0)).a <= 0.0f)
		return currentColor;

	float4 roughness = gSpecularRoughnessMap.Load(uint3(texcoord, 0)).a;
	float shininess = 1.0f - roughness;
	float4 bluredSsr = gBluredSsrMap.Load(uint3(texcoord, 0));

	return lerp(currentColor, bluredSsr, bluredSsr.a * shininess);
}