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
	// 카메라와 타일 중점의 거리를 계산한다.
	float d = distance(from, to);

	// 그 거리를 0.0(최소 거리)에서 1.0(최대 거리)구간으로 비례한다.
	return saturate((gMaxDistance - d) / (gMaxDistance - gMinDistance));
}

float ComputePatchLOD(float3 midPoint)
{
	// 비례된 거리를 계산한다.
	float d = ComputeScaledDistance(gEyePosW, midPoint);

	// [0, 1]구간 거리를 원하는 LOD구간으로 비례시킨다.
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
	// 32x32 그리드의 경우, 1024개의 패치가 있다.
	// 따라서 패치는 0~1023이고, 이를 XY좌표로 디코딩해야 한다.
	uint2 p;

	p.x = primitiveID % (uint)gGeometryDimension.x;
	p.y = primitiveID / (uint)gGeometryDimension.y;

	// 패치에 대한 XY좌표를 렌더링한 후 파라미터에 따라 오프셋해야 한다.
	p.x = clamp(p.x + xOffset, 0, ((uint)gGeometryDimension.x) - 1);
	p.y = clamp(p.y + zOffset, 0, ((uint)gGeometryDimension.y) - 1);

	return p;
}

float ComputePatchLODUsingLookup(float3 midPoint, float4 lookup)
{
	// 비례된 거리를 계산한다.
	float d = ComputeScaledDistance(gEyePosW, midPoint);

	// lookup:
	//	x,y,z = 패치 노멀
	//	w = 패치 표면에서의 표준 편차

	// 0.75 이상의 표준 편차를 얻는 것은 매우 드물다.
	// 따라서 여기에 작은 퍼지 팩터를 적용하여 선택한
	// 데이터 셋을 수정하거나 제거할 수 있다.
	float sd = lookup.w / 0.75;

	// 최종 LOD는 뷰어로부터 거리에 비례된 패치 당 표준 편차이다. 
	// 이 계산은 LOD를 그 이상으로 증가시키진 않지만 필요하지
	// 않는 패치에 대해선 추가 삼각형이 생기지 않도록 하는데에
	// 효과적일 것이다.
	float lod = clamp(sd * d, 0.0f, 1.0f);

	return lerp(gMinLOD, gMaxLOD, lod);
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// 세계 공간으로 변환한다.
	vout.mPosW = mul(float4(vin.mPosL, 1.0f), gTerrainWorld).xyz;

	// 자료를 그대로 덮개 셰이더에 넘겨준다.
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
		// 주 사각형
		ReadLookup(ComputeLookupIndex(patchID, 0, 0)),

		// +X 방향 이웃
		ReadLookup(ComputeLookupIndex(patchID, 0, 1)),

		// +Z 방향 이웃
		ReadLookup(ComputeLookupIndex(patchID, 1, 0)),

		// -X 방향 이웃
		ReadLookup(ComputeLookupIndex(patchID, 0, -1)),

		// -Z 방향 이웃
		ReadLookup(ComputeLookupIndex(patchID, -1, 0))
	};

	// 현재 타일과 이웃 타일들의 중점을 구한다.
	float3 midPoints[] =
	{
		// 주 사각형
		ComputePatchMidPoint(patch[0].mPosW, patch[1].mPosW, patch[2].mPosW, patch[3].mPosW),

		// +X 방향 이웃
		ComputePatchMidPoint(patch[2].mPosW, patch[3].mPosW, patch[4].mPosW, patch[5].mPosW),

		// +Z 방향 이웃
		ComputePatchMidPoint(patch[1].mPosW, patch[3].mPosW, patch[6].mPosW, patch[7].mPosW),

		// -X 방향 이웃
		ComputePatchMidPoint(patch[0].mPosW, patch[1].mPosW, patch[8].mPosW, patch[9].mPosW),

		// -Z 방향 이웃
		ComputePatchMidPoint(patch[0].mPosW, patch[2].mPosW, patch[10].mPosW, patch[11].mPosW)
	};

	/*
	// 각 타일의 카메라와의 거리를 이용해 적절한 LOD를 결정한다.
	float detail[] =
	{
		// 현재 타일 사각형
		ComputePatchLOD(midPoints[0]),

		// +X 방향 이웃
		ComputePatchLOD(midPoints[1]),

		// +Z 방향 이웃
		ComputePatchLOD(midPoints[2]),

		// -X 방향 이웃
		ComputePatchLOD(midPoints[3]),

		// -Z 방향 이웃
		ComputePatchLOD(midPoints[4])
	};
	*/

	// 각 타일의 표준 편차를 이용하여 적절한 LOD를 결정한다.
	float detail[] =
	{
		// 현재 타일 사각형
		ComputePatchLODUsingLookup(midPoints[0], tileLookup[0]),

		// +X 방향 이웃
		ComputePatchLODUsingLookup(midPoints[1], tileLookup[1]),

		// +Z 방향 이웃
		ComputePatchLODUsingLookup(midPoints[2], tileLookup[2]),

		// -X 방향 이웃
		ComputePatchLODUsingLookup(midPoints[3], tileLookup[3]),

		// -Z 방향 이웃
		ComputePatchLODUsingLookup(midPoints[4], tileLookup[4])
	};

	// 현재 타일 내부 조각의 LOD는 항상 타일 자체의 LOD와 같다.
	pt.mInsideTess[0] = pt.mInsideTess[1] = detail[0];

	// 이웃 타일의 LOD가 더 낮다면 그 LOD를 가장자리 조각의 LOD로 설정한다.
	// 이웃 타일의 LOD가 더 높다면 가장자리 조각에 그대로 사용한다.
	// (그 이웃 타일이 현재 타일에 맞게 자신의 가장자리 LOD를 낮춘다.)
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

	// patch[]에서 뽑은 세계 공간 좌표 성분 세 개와 uvw(무게 중심 좌표)
	// 보간 계수들을 이용해서 위치를 적절히 보간한다.

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

	// 세계 공간에서의 좌표를 전달한다.
	dout.mPosW = finalVertexCoord;
	// 동차 절단 공간으로 변환한다.
	dout.mPosH = mul(float4(finalVertexCoord, 1.0f), gViewProj);
	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
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

	// 미리 계산된 노멀맵에서 해당 픽셀 위치의 노멀을 가져온다.
	float3 normal = normalize(gNormalMap.SampleLevel(gsamAnisotropicWrap, pin.mTexC, 0.0f).xyz);

	// Terrain의 절반이상부터는 꼭대기에 눈이 쌓인듯한 효과를 준다.
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