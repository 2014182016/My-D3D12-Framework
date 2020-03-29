#include "Defines.hlsl"
#include "Particle.hlsl"

const static float2 gTexC[4] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 1.0f),
	float2(1.0f, 0.0f)
};

StructuredBuffer<ParticleData>  particles : register(t0);
Texture2D gTextureMaps[TEX_NUM] : register(t0, space1);

SamplerState gsamLinearWrap : register(s0);

struct VertexOut
{
	float3 posW   : POSITION;
	float4 color  : COLOR;
	float2 size   : SIZE;
	bool isActive : ACTIVE;
};

struct GeoOut
{
	float4 posH   : SV_POSITION;
	float3 posW   : POSITION;
	float3 normal : NORMAL;
	float4 color  : COLOR;
	float2 texC   : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout = (VertexOut)0.0f;

	if (particles[vid].isActive == true)
	{
		vout.posW = particles[vid].position;
		vout.color = particles[vid].color;
		vout.size = particles[vid].size;
		vout.isActive = true;
	}

	return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1],
	inout TriangleStream<GeoOut> triStream)
{
	if (gin[0].isActive == false)
		return;

	// �����尡 xz ��鿡 �پ y �������� ������ ���¿��� ī�޶�
	// �׻� ���ϰ� ����� ���� ���� ���� ������ ���� ��ǥ�踦 ����Ѵ�.
	float3 centerV = mul(float4(gin[0].posW, 1.0f), gView).xyz;
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].posW;
	look = normalize(look);
	float3 right = cross(up, look);

	// ���� ���� ������ �ﰢ�� �� ������(�簢���� �����ϴ�)�� ����Ѵ�.
	float halfWidth = 0.5f * gin[0].size.x;
	float halfHeight = 0.5f * gin[0].size.y;

	// �� ������ ���� ��ǥ�� ����Ѵ�.
	float4 v[4];
	v[0] = float4(gin[0].posW + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin[0].posW + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin[0].posW - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin[0].posW - halfWidth * right + halfHeight * up, 1.0f);

	GeoOut gout;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gout.posH = mul(v[i], gViewProj);
		gout.posW = v[i].xyz;
		gout.normal = look;
		gout.texC = gTexC[i];
		gout.color = gin[0].color;

		triStream.Append(gout);
	}

	triStream.RestartStrip();
}

float4 PS(GeoOut pin) : SV_Target
{
	float4 color = 0.0f;
	if (gTextureIndex != DISABLED)
	{
		color = gTextureMaps[gTextureIndex].Sample(gsamLinearWrap, pin.texC);
		color *= pin.color;
	}
	else
	{
		color = pin.color;
	}

	return color;
}