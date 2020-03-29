static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float2 texC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.texC = gTexCoords[vid];

	// ��ũ�� ��ǥ�迡�� ���� ��ǥ��� ��ȯ�Ѵ�.
	vout.posH = float4(2.0f * vout.texC.x - 1.0f, 1.0f - 2.0f * vout.texC.y, 0.0f, 1.0f);

	return vout;
}