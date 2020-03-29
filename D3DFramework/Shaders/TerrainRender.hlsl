#include "LightingUtil.hlsl"
#include "Pass.hlsl"

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

struct VertexIn
{
	float3 posL : POSITION;
	float2 texC : TEXCOORD;
};

struct VertexOut
{
	float3 posW : POSITION;
	float2 texC : TEXCOORD;
};

struct HullOut
{
	float3 posW : POSITION;
	float2 texC : TEXCOORD;
};

struct PatchTess
{
	float edgeTess[4]   : SV_TessFactor;
	float insideTess[2] : SV_InsideTessFactor;
};

struct DomainOut
{
	float4 posH   : SV_POSITION;
	float3 posW   : POSITION;
	float2 texC   : TEXCOORD;
	float  height : HEIGHT;
};

struct PixelOut
{
	float4 diffuse  : SV_TARGET0;
	float4 specularRoughness : SV_TARGET1;
	float4 position : SV_TARGET2;
	float4 normal   : SV_TARGET3;
	float4 normalx  : SV_TARGET4;
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
	uint heightMapIndex = gMaterialData[gMaterialIndex].heightMapIndex;
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
	vout.posW = mul(float4(vin.posL, 1.0f), gTerrainWorld).xyz;

	// �ڷḦ �״�� ���� ���̴��� �Ѱ��ش�.
	vout.texC = vin.texC;

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

	hout.posW = p[i].posW;
	hout.texC = p[i].texC;

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
		ComputePatchMidPoint(patch[0].posW, patch[1].posW, patch[2].posW, patch[3].posW),

		// +X ���� �̿�
		ComputePatchMidPoint(patch[2].posW, patch[3].posW, patch[4].posW, patch[5].posW),

		// +Z ���� �̿�
		ComputePatchMidPoint(patch[1].posW, patch[3].posW, patch[6].posW, patch[7].posW),

		// -X ���� �̿�
		ComputePatchMidPoint(patch[0].posW, patch[1].posW, patch[8].posW, patch[9].posW),

		// -Z ���� �̿�
		ComputePatchMidPoint(patch[0].posW, patch[2].posW, patch[10].posW, patch[11].posW)
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
	pt.insideTess[0] = pt.insideTess[1] = detail[0];

	// �̿� Ÿ���� LOD�� �� ���ٸ� �� LOD�� �����ڸ� ������ LOD�� �����Ѵ�.
	// �̿� Ÿ���� LOD�� �� ���ٸ� �����ڸ� ������ �״�� ����Ѵ�.
	// (�� �̿� Ÿ���� ���� Ÿ�Ͽ� �°� �ڽ��� �����ڸ� LOD�� �����.)
	pt.edgeTess[0] = min(detail[0], detail[4]);
	pt.edgeTess[1] = min(detail[0], detail[3]);
	pt.edgeTess[2] = min(detail[0], detail[2]);
	pt.edgeTess[3] = min(detail[0], detail[1]);

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

	float3 finalVertexCoord = quad[0].posW * (1.0f - uv.x) * (1.0f - uv.y) +
		quad[1].posW * uv.x * (1.0f - uv.y) +
		quad[2].posW * (1.0f - uv.x) * uv.y +
		quad[3].posW * uv.x * uv.y;

	float2 texCoord = quad[0].texC * (1.0f - uv.x) * (1.0f - uv.y) +
		quad[1].texC * uv.x * (1.0f - uv.y) +
		quad[2].texC * (1.0f - uv.x) * uv.y +
		quad[3].texC * uv.x * uv.y;

	dout.height = GetHeightMapSample(texCoord);
	finalVertexCoord.y += dout.height * gHeightScale;

	// ���� ���������� ��ǥ�� �����Ѵ�.
	dout.posW = finalVertexCoord;
	// ���� ���� �������� ��ȯ�Ѵ�.
	dout.posH = mul(float4(finalVertexCoord, 1.0f), gViewProj);

	float4x4 matTransform;
	if (gMaterialIndex == DISABLED)
		matTransform = gIdentity;
	else
		matTransform = gMaterialData[gMaterialIndex].matTransform;

	// ��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	dout.texC = mul(float4(texCoord, 0.0f, 1.0f), matTransform).xy;

	return dout;
}

PixelOut PS(DomainOut pin)
{
	PixelOut pout = (PixelOut)0.0f;

	// �� �ȼ��� ����� ���͸��� �����͸� �����´�.
	MaterialData materialData = gMaterialData[gMaterialIndex];

	float4 diffuse = materialData.diffuseAlbedo;
	float3 specular = materialData.specular;
	float roughness = materialData.roughness;
	uint diffuseMapIndex = materialData.diffuseMapIndex;

	if (diffuseMapIndex != DISABLED)
	{
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC * gTexScale);
	}

	// �̸� ���� ��ָʿ��� �ش� �ȼ� ��ġ�� ����� �����´�.
	float3 normal = normalize(gNormalMap.SampleLevel(gsamAnisotropicWrap, pin.texC, 0.0f).xyz);

	// Terrain�� �����̻���ʹ� ����⿡ ���� ���ε��� ȿ���� �ش�.
	if (pin.height > 0.5f)
	{
		float snowAmount = smoothstep(0.5f, 1.0f, pin.height);
		diffuse = lerp(diffuse, 1.0f, snowAmount);
	}

	// ���� ��Ƽ ���������� G���۸� ä���.
	pout.diffuse = float4(diffuse.rgb, 1.0f);
	pout.specularRoughness = float4(specular, roughness);
	pout.position = float4(pin.posW, 0.0f);
	pout.normal = float4(normal, 1.0f);

	return pout;
}

float4 PS_Wireframe(DomainOut pin) : SV_Target
{
	return gMaterialData[gMaterialIndex].diffuseAlbedo;
}