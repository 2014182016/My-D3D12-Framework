#include "Common.hlsl"

static float4 gTopColor = float4(0.0f, 0.05f, 0.6f, 1.0f);
static float4 gCenterColor = float4(0.7f, 0.7f, 0.92f, 1.0f);
static float gSkyScale = 5000.0f;

struct VertexIn
{
	float3 posL : POSITION;
	float2 texC : TEXCOORD;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float3 posW : POSITION;
	float2 texC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// World Space�� ��ȯ�Ѵ�.
	float4 posW = mul(float4(vin.posL, 1.0f), gObjWorld);
	vout.posW = posW.xyz;

	// z = w�̸� �ش� �ȼ��� �׻� far plane�� ��ġ���ִ�.
	vout.posH = mul(posW, gViewProj).xyww;

	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.texC = mul(float4(vin.texC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

[earlydepthstencil]
float4 PS(VertexOut pin) : SV_Target
{
	uint diffuseMapIndex = gMaterialData[gObjMaterialIndex].diffuseMapIndex;
	float diffuseIntensity = 1.0f;

	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
	if (diffuseMapIndex != DISABLED)
	{
		// �ؽ�ó�� R������ ������ ���� �κи��� �������ϵ��� �Ѵ�.
		diffuseIntensity = gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC).r;
	}

	// �ϴ��� ���̴� [0, 1]���̷� ����ȭ�Ѵ�.
	float height = pin.posW.y / gSkyScale;
	if (height < 0.0f)
	{
		// ���� �Ʒ��� ��Ī���·� �������ȴ�.
		height *= -1.0f;
	}
	
	// ���̿� ���� ����� �κа� �߽� �κ��� ���� ���´�.
	float4 skyColor = lerp(gTopColor, gCenterColor, height);
	// �ϴÿ� ������ �������Ѵ�.
	float4 outColor = skyColor + diffuseIntensity;

	return outColor;
}

