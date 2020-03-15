#include "LightingUtil.hlsl"

Texture2D gTextureMaps[TEX_NUM] : register(t0, space0);
Texture2D gLODLookupMap : register(t0, space1);
Texture2D gNormalMap : register(t1, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t2, space1);

SamplerState gsamLinearClamp      : register(s0);
SamplerState gsamAnisotropicWrap  : register(s1);

cbuffer cbTerrain : register(b0)
{
	float4x4 gTerrainWorld;
	float2 gPixelDimension;
	float2 gGeometryDimension;
	float gMaxLOD;
	float gMinLOD;
	float gMinDistance;
	float gMaxDistance;
	float gHeightScale;
	float gTexScale;
	uint gMaterialIndex;
	float gTPadding0;
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
	float4 gAmbientLight;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float3 gEyePosW;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	bool gFogEnabled;
	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float gFogDensity;
	uint gFogType;
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
	float4 mPosH   : SV_POSITION;
	float3 mPosW   : POSITION;
	float2 mTexC   : TEXCOORD;
	float  mHeight : HEIGHT;
};

struct PixelOut
{
	float4 mDiffuse  : SV_TARGET0;
	float4 mSpecularRoughness : SV_TARGET1;
	float4 mPosition : SV_TARGET2;
	float4 mNormal   : SV_TARGET3;
	float4 mNormalx  : SV_TARGET4;
};

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
	uint heightMapIndex = gMaterialData[gMaterialIndex].mHeightMapIndex;
	return gTextureMaps[heightMapIndex].SampleLevel(gsamLinearClamp, uv, 0.0f).r;
}

float4 ReadLookup(uint2 idx)
{
	return gLODLookupMap.Load(uint3(idx, 0));
}

uint2 ComputeLookupIndex(uint primitiveID, int xOffset, int zOffset)
{
	// 32x32 �׸����� ���, 1024���� ��ġ�� �ִ�.
	// ���� ��ġ�� 0~1023�̰�, �̸� XY��ǥ�� ���ڵ��ؾ� �Ѵ�.
	uint2 p;

	p.x = primitiveID % (uint)gGeometryDimension.x;
	p.y = primitiveID / (uint)gGeometryDimension.y;

	// ��ġ�� ���� XY��ǥ�� �������� �� �Ķ���Ϳ� ���� �������ؾ� �Ѵ�.
	p.x = clamp(p.x + xOffset, 0, ((uint)gGeometryDimension.x) - 1);
	p.y = clamp(p.y + zOffset, 0, ((uint)gGeometryDimension.y) - 1);

	return p;
}

float ComputePatchLODUsingLookup(float3 midPoint, float4 lookup)
{
	// ��ʵ� �Ÿ��� ����Ѵ�.
	float d = ComputeScaledDistance(gEyePosW, midPoint);

	// lookup:
	//	x,y,z = ��ġ ���
	//	w = ��ġ ǥ�鿡���� ǥ�� ����

	// 0.75 �̻��� ǥ�� ������ ��� ���� �ſ� �幰��.
	// ���� ���⿡ ���� ���� ���͸� �����Ͽ� ������
	// ������ ���� �����ϰų� ������ �� �ִ�.
	float sd = lookup.w / 0.75;

	// ���� LOD�� ���κ��� �Ÿ��� ��ʵ� ��ġ �� ǥ�� �����̴�. 
	// �� ����� LOD�� �� �̻����� ������Ű�� ������ �ʿ�����
	// �ʴ� ��ġ�� ���ؼ� �߰� �ﰢ���� ������ �ʵ��� �ϴµ���
	// ȿ������ ���̴�.
	float lod = clamp(sd * d, 0.0f, 1.0f);

	return lerp(gMinLOD, gMaxLOD, lod);
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

	float4 tileLookup[] =
	{
		// �� �簢��
		ReadLookup(ComputeLookupIndex(patchID, 0, 0)),

		// +X ���� �̿�
		ReadLookup(ComputeLookupIndex(patchID, 0, 1)),

		// +Z ���� �̿�
		ReadLookup(ComputeLookupIndex(patchID, 1, 0)),

		// -X ���� �̿�
		ReadLookup(ComputeLookupIndex(patchID, 0, -1)),

		// -Z ���� �̿�
		ReadLookup(ComputeLookupIndex(patchID, -1, 0))
	};

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

	/*
	// �� Ÿ���� ī�޶���� �Ÿ��� �̿��� ������ LOD�� �����Ѵ�.
	float detail[] =
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
	*/

	// �� Ÿ���� ǥ�� ������ �̿��Ͽ� ������ LOD�� �����Ѵ�.
	float detail[] =
	{
		// ���� Ÿ�� �簢��
		ComputePatchLODUsingLookup(midPoints[0], tileLookup[0]),

		// +X ���� �̿�
		ComputePatchLODUsingLookup(midPoints[1], tileLookup[1]),

		// +Z ���� �̿�
		ComputePatchLODUsingLookup(midPoints[2], tileLookup[2]),

		// -X ���� �̿�
		ComputePatchLODUsingLookup(midPoints[3], tileLookup[3]),

		// -Z ���� �̿�
		ComputePatchLODUsingLookup(midPoints[4], tileLookup[4])
	};

	// ���� Ÿ�� ���� ������ LOD�� �׻� Ÿ�� ��ü�� LOD�� ����.
	pt.mInsideTess[0] = pt.mInsideTess[1] = detail[0];

	// �̿� Ÿ���� LOD�� �� ���ٸ� �� LOD�� �����ڸ� ������ LOD�� �����Ѵ�.
	// �̿� Ÿ���� LOD�� �� ���ٸ� �����ڸ� ������ �״�� ����Ѵ�.
	// (�� �̿� Ÿ���� ���� Ÿ�Ͽ� �°� �ڽ��� �����ڸ� LOD�� �����.)
	pt.mEdgeTess[0] = min(detail[0], detail[4]);
	pt.mEdgeTess[1] = min(detail[0], detail[3]);
	pt.mEdgeTess[2] = min(detail[0], detail[2]);
	pt.mEdgeTess[3] = min(detail[0], detail[1]);

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

	dout.mHeight = GetHeightMapSample(texCoord);
	finalVertexCoord.y += dout.mHeight * gHeightScale;

	// ���� ���������� ��ǥ�� �����Ѵ�.
	dout.mPosW = finalVertexCoord;
	// ���� ���� �������� ��ȯ�Ѵ�.
	dout.mPosH = mul(float4(finalVertexCoord, 1.0f), gViewProj);
	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	dout.mTexC = mul(float4(texCoord, 0.0f, 1.0f), gMaterialData[gMaterialIndex].mMatTransform).xy;

	return dout;
}

PixelOut PS(DomainOut pin)
{
	PixelOut pout = (PixelOut)0.0f;

	MaterialData materialData = gMaterialData[gMaterialIndex];
	float4 diffuse = materialData.mDiffuseAlbedo;
	float3 specular = materialData.mSpecular;
	float roughness = materialData.mRoughness;
	uint diffuseMapIndex = materialData.mDiffuseMapIndex;

	if (diffuseMapIndex != DISABLED)
	{
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.mTexC * gTexScale);
	}

	// �̸� ���� ��ָʿ��� �ش� �ȼ� ��ġ�� ����� �����´�.
	float3 normal = normalize(gNormalMap.SampleLevel(gsamAnisotropicWrap, pin.mTexC, 0.0f).xyz);

	// Terrain�� �����̻���ʹ� ����⿡ ���� ���ε��� ȿ���� �ش�.
	if (pin.mHeight > 0.5f)
	{
		float snowAmount = smoothstep(0.5f, 1.0f, pin.mHeight);
		diffuse = lerp(diffuse, 1.0f, snowAmount);
	}

	pout.mDiffuse = float4(diffuse.rgb, 1.0f);
	pout.mSpecularRoughness = float4(specular, roughness);
	pout.mPosition = float4(pin.mPosW, 0.0f);
	pout.mNormal = float4(normal, 1.0f);

	return pout;
}

float4 PS_Wireframe(DomainOut pin) : SV_Target
{
	MaterialData materialData = gMaterialData[gMaterialIndex];
	return materialData.mDiffuseAlbedo;
}