#include "LightingUtil.hlsl"

Texture2D gTextureMaps[TEX_NUM] : register(t0, space0);
StructuredBuffer<Light> gLights : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

SamplerState gsamLinearClamp      : register(s0);
SamplerState gsamAnisotropicWrap  : register(s1);

cbuffer cbTerrain : register(b0)
{
	float4x4 gTerrainWorld;
	float gMaxLOD;
	float gMinLOD;
	float gMinDistance;
	float gMaxDistance;
	float gHeightScale;
	float gTexScale;
	uint gMaterialIndex;
	uint gHeightMapIndex;
}

cbuffer cbPass : register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gProjTex;
	float4x4 gViewProjTex;
	float4x4 gIdentity;
	float3 gEyePosW;
	float gPadding1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;
	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float gFogDensity;
	bool gFogEnabled;
	uint gFogType;
	float gSsaoContrast;
	float gPadding2;
	float gPadding3;
};

struct VertexIn
{
	float3 mPosL : POSITION;
	float2 mTexC : TEXCOORD;
};

struct VertexOut
{
	float3 mPosW : POSITION;
	float2 mTexC : TEXCOORD;
};

struct HullOut
{
	float3 mPosW : POSITION;
	float2 mTexC : TEXCOORD;
};

struct PatchTess
{
	float mEdgeTess[4]   : SV_TessFactor;
	float mInsideTess[2] : SV_InsideTessFactor;
};

struct DomainOut
{
	float4 mPosH  : SV_POSITION;
	float3 mPosW  : POSITION;
	float2 mTexC  : TEXCOORD;
	float3 mNormal: NORMAL;
};

float4 GetFogBlend(float4 litColor, float distToEye)
{
	float4 result = 0.0f;

	switch (gFogType)
	{
	case FOG_LINEAR:
	{
		// �Ÿ��� ���� �Ȱ� ������ ���� ����� ����Ѵ�.
		float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
		result = lerp(litColor, gFogColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL:
	{
		// ���������� �ָ� �������� �Ȱ��� �� �β�������.
		float fogAmount = exp(-distToEye * gFogDensity);
		result = lerp(gFogColor, litColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL2:
	{
		// �ſ� �β��� �Ȱ��� ��Ÿ����.
		float fogAmount = exp(-pow(distToEye * gFogDensity, 2));
		result = lerp(gFogColor, litColor, fogAmount);
		break;
	}
	}

	return result;
}

float3 ComputePatchMidPoint(float3 p0, float3 p1, float3 p2, float3 p3)
{
	return (p0 + p1 + p2 + p3) / 4.0f;
}

float ComputeScaledDistance(float3 from, float3 to)
{
	// ī�޶�� Ÿ�� ������ �Ÿ��� ����Ѵ�.
	float d = distance(from, to);

	// �� �Ÿ��� 0.0(�ּ� �Ÿ�)���� 1.0(�ִ� �Ÿ�)�������� ����Ѵ�.
	return saturate((gMaxDistance - d) / (gMaxDistance - gMinDistance));
}

float ComputePatchLOD(float3 midPoint)
{
	// ��ʵ� �Ÿ��� ����Ѵ�.
	float d = ComputeScaledDistance(gEyePosW, midPoint);

	// [0, 1]���� �Ÿ��� ���ϴ� LOD�������� ��ʽ�Ų��.
	return lerp(gMinLOD, gMaxLOD, d);
}

float GetHeightMapSample(float2 uv)
{
	return gHeightScale * gTextureMaps[gHeightMapIndex].SampleLevel(gsamLinearClamp, uv, 0.0f).r;
}

float3 Sobel(float2 uv)
{
	float2 pxSz = float2(1.0f / 512.0f, 1.0f / 512.0f);

	// �ʿ��� �����µ��� ����Ѵ�.
	float2 o00 = uv + float2(-pxSz.x, -pxSz.y);
	float2 o10 = uv + float2(0.0f, -pxSz.y);
	float2 o20 = uv + float2(pxSz.x, -pxSz.y);
	float2 o01 = uv + float2(-pxSz.x, 0.0f);
	float2 o02 = uv + float2(-pxSz.x, pxSz.y);
	float2 o12 = uv + float2(0.0f, pxSz.y);
	float2 o21 = uv + float2(pxSz.x, 0.0f);
	float2 o22 = uv + float2(pxSz.x, pxSz.y);

	// �Һ� ���͸� �����Ϸ��� ���� �ʼ� ������ ���� �ȼ��� �ʿ��Ѵ�.
	float h00 = GetHeightMapSample(o00);
	float h10 = GetHeightMapSample(o10);
	float h20 = GetHeightMapSample(o20);
	float h01 = GetHeightMapSample(o01);
	float h02 = GetHeightMapSample(o02);
	float h12 = GetHeightMapSample(o12);
	float h21 = GetHeightMapSample(o21);
	float h22 = GetHeightMapSample(o22);

	// �Һ� ���͵��� ���Ѵ�.
	float gx = h00 - h20 + 2.0f * h01 - 2.0f * h21 + h02 - h22;
	float gy = h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;

	// ���� z ������ �����Ѵ�.
	float gz = 0.01f * sqrt(max(0.0f, 1.0f - gx * gx - gy * gy));

	return normalize(float3(2.0f * gx, gz, 2.0f * gy));
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// ���� �������� ��ȯ�Ѵ�.
	vout.mPosW = mul(float4(vin.mPosL, 1.0f), gTerrainWorld).xyz;

	// �ڷḦ �״�� ���� ���̴��� �Ѱ��ش�.
	vout.mTexC = vin.mTexC;

	return vout;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HsPerPatch")]
HullOut HS(InputPatch<VertexOut, 12> p, uint i : SV_OutputControlPointID)
{
	HullOut hout = (HullOut)0.0f;

	hout.mPosW = p[i].mPosW;
	hout.mTexC = p[i].mTexC;

	return hout;
}

PatchTess HsPerPatch(InputPatch<VertexOut, 12> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt = (PatchTess)0.0f;

	// ���� Ÿ�ϰ� �̿� Ÿ�ϵ��� ������ ���Ѵ�.
	float3 midPoints[] =
	{
		// �� �簢��
		ComputePatchMidPoint(patch[0].mPosW, patch[1].mPosW, patch[2].mPosW, patch[3].mPosW),

		// +X ���� �̿�
		ComputePatchMidPoint(patch[2].mPosW, patch[3].mPosW, patch[4].mPosW, patch[5].mPosW),

		// +Z ���� �̿�
		ComputePatchMidPoint(patch[1].mPosW, patch[3].mPosW, patch[6].mPosW, patch[7].mPosW),

		// -X ���� �̿�
		ComputePatchMidPoint(patch[0].mPosW, patch[1].mPosW, patch[8].mPosW, patch[9].mPosW),

		// -Z ���� �̿�
		ComputePatchMidPoint(patch[0].mPosW, patch[2].mPosW, patch[10].mPosW, patch[11].mPosW)
	};

	// �� Ÿ���� ������ LOD�� �����Ѵ�.
	float dist[] =
	{
		// ���� Ÿ�� �簢��
		ComputePatchLOD(midPoints[0]),

		// +X ���� �̿�
		ComputePatchLOD(midPoints[1]),

		// +Z ���� �̿�
		ComputePatchLOD(midPoints[2]),

		// -X ���� �̿�
		ComputePatchLOD(midPoints[3]),

		// -Z ���� �̿�
		ComputePatchLOD(midPoints[4])
	};

	// ���� Ÿ�� ���� ������ LOD�� �׻� Ÿ�� ��ü�� LOD�� ����.
	pt.mInsideTess[0] = pt.mInsideTess[1] = dist[0];

	// �̿� Ÿ���� LOD�� �� ���ٸ� �� LOD�� �����ڸ� ������ LOD�� �����Ѵ�.
	// �̿� Ÿ���� LOD�� �� ���ٸ� �����ڸ� ������ �״�� ����Ѵ�.
	// (�� �̿� Ÿ���� ���� Ÿ�Ͽ� �°� �ڽ��� �����ڸ� LOD�� �����.)
	pt.mEdgeTess[0] = min(dist[0], dist[4]);
	pt.mEdgeTess[1] = min(dist[0], dist[3]);
	pt.mEdgeTess[2] = min(dist[0], dist[2]);
	pt.mEdgeTess[3] = min(dist[0], dist[1]);

	return pt;
}

[domain("quad")]
DomainOut DS(PatchTess patchTess,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HullOut, 4> quad)
{
	DomainOut dout = (DomainOut)0.0f;

	// patch[]���� ���� ���� ���� ��ǥ ���� �� ���� uvw(���� �߽� ��ǥ)
	// ���� ������� �̿��ؼ� ��ġ�� ������ �����Ѵ�.

	// u,v
	// 0,0 : patchTess[0].mPos;
	// 1,0 : patchTess[1].mPos;
	// 0,1 : patchTess[2].mPos;
	// 1,1 : patchTess[3].mPos;

	float3 finalVertexCoord = quad[0].mPosW * (1.0f - uv.x) * (1.0f - uv.y) +
		quad[1].mPosW * uv.x * (1.0f - uv.y) +
		quad[2].mPosW * (1.0f - uv.x) * uv.y +
		quad[3].mPosW * uv.x * uv.y;

	float2 texCoord = quad[0].mTexC * (1.0f - uv.x) * (1.0f - uv.y) +
		quad[1].mTexC * uv.x * (1.0f - uv.y) +
		quad[2].mTexC * (1.0f - uv.x) * uv.y +
		quad[3].mTexC * uv.x * uv.y;

	finalVertexCoord.y += GetHeightMapSample(texCoord);

	// ���� ���������� ��ǥ�� �����Ѵ�.
	dout.mPosW = finalVertexCoord;
	// ���� ���� �������� ��ȯ�Ѵ�.
	dout.mPosH = mul(float4(finalVertexCoord, 1.0f), gViewProj);
	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	dout.mTexC = mul(float4(texCoord, 0.0f, 1.0f), gMaterialData[gMaterialIndex].mMatTransform).xy;
	// �Һ� �����ڸ� ����Ͽ� ����� ����Ѵ�.
	dout.mNormal = Sobel(texCoord);

	return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
	MaterialData materialData = gMaterialData[gMaterialIndex];
	float4 diffuse = materialData.mDiffuseAlbedo;
	float3 specular = materialData.mSpecular;
	float roughness = materialData.mRoughness;
	uint diffuseMapIndex = materialData.mDiffuseMapIndex;

	if (diffuseMapIndex != DISABLED)
	{
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC * gTexScale);
	}

	// ����Ǵ� �ȼ����� �������� ����
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse�� ���������� �����ִ� Ambient��
	float4 ambient = gAmbientLight * diffuse;

	// ����� ����ȭ�Ѵ�.
	pin.mNormal = normalize(pin.mNormal);

	// roughness�� normal�� �̿��Ͽ� shininess�� ����Ѵ�.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuse, specular, shininess };

	// Terrain���� gLight�� DirectionalLight���� �������Ѵ�.
	float4 directLight = ComputeOnlyDirectionalLight(gLights, mat, pin.mNormal, toEyeW);
	float4 litColor = ambient + directLight;

	if (gFogEnabled)
	{
		litColor = GetFogBlend(litColor, distToEye);
	}

	return litColor;
}