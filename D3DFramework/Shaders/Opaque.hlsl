#include "Common.hlsl"

struct VertexIn
{
	float3 posL     : POSITION;
	float3 normalL  : NORMAL;
	float3 tangentU : TANGENT;
	float3 binormalU: BINORMAL;
	float2 texC     : TEXCOORD;
};

struct VertexOut
{
	float4 posH      : SV_POSITION;
	float3 posW      : POSITION0;
	float3 normalW   : NORMAL;
	float3 tangentW  : TANGENT;
	float3 binormalW : BINORMAL;
	float2 texC      : TEXCOORD;
};

struct PixelOut
{
	float4 diffuse  : SV_TARGET0;
	float4 specularRoughness : SV_TARGET1;
	float4 position : SV_TARGET2;
	float4 normal   : SV_TARGET3;
	float4 normalx  : SV_TARGET4;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// World Space�� ��ȯ�Ѵ�.
	float4 posW = mul(float4(vin.posL, 1.0f), gObjWorld);
	vout.posW = posW.xyz;

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.posH = mul(posW, gViewProj);

	// World Matrix�� ��յ� ��ʰ� ���ٰ� �����ϰ� Normal�� ��ȯ�Ѵ�.
	// ��յ� ��ʰ� �ִٸ� ����ġ ����� ����ؾ� �Ѵ�.
	vout.normalW = mul(vin.normalL, (float3x3)gObjWorld);
	vout.tangentW = mul(vin.tangentU, (float3x3)gObjWorld);
	vout.binormalW = mul(vin.binormalU, (float3x3)gObjWorld);

	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.texC = mul(float4(vin.texC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

PixelOut PS(VertexOut pin)
{
	PixelOut pout = (PixelOut)0.0f;

	// �� �ȼ��� ����� ���͸��� �����͸� �����´�.
	MaterialData materialData = gMaterialData[gObjMaterialIndex];

	float4 diffuse = materialData.diffuseAlbedo;
	float3 specular = materialData.specular;
	float roughness = materialData.roughness;
	uint diffuseMapIndex = materialData.diffuseMapIndex;
	uint normalMapIndex = materialData.normalMapIndex;

	if (diffuseMapIndex != DISABLED)
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC);

	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
	clip(diffuse.a - 0.1f);

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
	pin.normalW = normalize(pin.normalW);
	pin.tangentW = normalize(pin.tangentW);
	pin.binormalW = normalize(pin.binormalW);

	// ��ָ��� ������� ���� ����� �����Ѵ�.
	pout.normalx = float4(pin.normalW, 1.0f);

	float3 bumpedNormalW = pin.normalW;

	if (normalMapIndex != DISABLED)
	{
		float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.texC);

		// ��ָʿ��� Tangent Space�� ����� World Space�� ��ַ� ��ȯ�Ѵ�.
		float3 normalT = 2.0f * normalMapSample.rgb - 1.0f;
		float3x3 tbn = float3x3(pin.tangentW, pin.binormalW, pin.normalW);
		bumpedNormalW = mul(normalT, tbn);
		bumpedNormalW = normalize(bumpedNormalW);
	}

	// ���� ��Ƽ ���������� G���۸� ä���.
	pout.diffuse = float4(diffuse.rgb, 1.0f);
	pout.specularRoughness = float4(specular, roughness);
	pout.position = float4(pin.posW, 1.0f);
	pout.normal = float4(bumpedNormalW, 1.0f);

	return pout;
}