#include "Common.hlsl"

struct VertexIn
{
	float3 mPosL     : POSITION;
    float3 mNormalL  : NORMAL;
	float3 mTangentU : TANGENT;
	float3 mBinormalU: BINORMAL;
	float2 mTexC     : TEXCOORD;
};

struct VertexOut
{
	float4 mPosH      : SV_POSITION;
    float3 mPosW      : POSITION0;
    float3 mNormalW   : NORMAL;
	float3 mTangentW  : TANGENT;
	float3 mBinormalW : BINORMAL;
	float2 mTexC      : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // World Space�� ��ȯ�Ѵ�.
    float4 posW = mul(float4(vin.mPosL, 1.0f), gObjWorld);
    vout.mPosW = posW.xyz;

	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.mPosH = mul(posW, gViewProj);

	// World Matrix�� ��յ� ��ʰ� ���ٰ� �����ϰ� Normal�� ��ȯ�Ѵ�.
	// ��յ� ��ʰ� �ִٸ� ����ġ ����� ����ؾ� �Ѵ�.
    vout.mNormalW = mul(vin.mNormalL, (float3x3)gObjWorld);
	vout.mTangentW = mul(vin.mTangentU, (float3x3)gObjWorld);
	vout.mBinormalW = mul(vin.mBinormalU, (float3x3)gObjWorld);
	
	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	vout.mTexC = mul(float4(vin.mTexC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuse = 0.0f; float3 specular = 0.0f; float roughness = 0.0f;
	GetMaterialAttibute(gObjMaterialIndex, diffuse, specular, roughness);

	diffuse *= GetDiffuseMapSample(gObjMaterialIndex, pin.mTexC);

    // �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. 
    clip(diffuse.a - 0.1f);

	// ������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
    pin.mNormalW = normalize(pin.mNormalW);
	pin.mTangentW = normalize(pin.mTangentW);
	pin.mBinormalW = normalize(pin.mBinormalW);
	float3 bumpedNormalW = pin.mNormalW;
	
	float4 normalMapSample = GetNormalMapSample(gObjMaterialIndex, pin.mTexC);
	if (!any(normalMapSample))
	{
		// ��ָʿ��� Tangent Space�� ����� World Space�� ��ַ� ��ȯ�Ѵ�.
		float3 normalT = 2.0f * normalMapSample.rgb - 1.0f;
		float3x3 tbn = float3x3(pin.mTangentW, pin.mBinormalW, pin.mNormalW);
		bumpedNormalW = mul(normalT, tbn);
		bumpedNormalW = normalize(bumpedNormalW);
	}

    // ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse�� ���������� �����ִ� Ambient��
	float4 ambient = gAmbientLight * diffuse;

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;
    Material mat = { diffuse, specular, shininess };

	// Lighting�� �ǽ��Ѵ�.
    float4 directLight = ComputeShadowLighting(gLights, mat, pin.mPosW, bumpedNormalW, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	litColor *= GetAmbientAccess(pin.mPosW);
#endif

	// �л� �������� ���ĸ� �����´�.
	litColor.a = diffuse.a;

	if (gFogEnabled)
	{
		litColor = GetFogBlend(litColor, distToEye);
	}

    return litColor;
}


