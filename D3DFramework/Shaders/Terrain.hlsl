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
		// 거리에 따라 안개 색상을 선형 감쇠로 계산한다.
		float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
		result = lerp(litColor, gFogColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL:
	{
		// 지수적으로 멀리 있을수록 안개가 더 두꺼워진다.
		float fogAmount = exp(-distToEye * gFogDensity);
		result = lerp(gFogColor, litColor, fogAmount);
		break;
	}
	case FOG_EXPONENTIAL2:
	{
		// 매우 두꺼운 안개를 나타낸다.
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
	return gHeightScale * gTextureMaps[gHeightMapIndex].SampleLevel(gsamLinearClamp, uv, 0.0f).r;
}

float3 Sobel(float2 uv)
{
	float2 pxSz = float2(1.0f / 512.0f, 1.0f / 512.0f);

	// 필요한 오프셋들을 계산한다.
	float2 o00 = uv + float2(-pxSz.x, -pxSz.y);
	float2 o10 = uv + float2(0.0f, -pxSz.y);
	float2 o20 = uv + float2(pxSz.x, -pxSz.y);
	float2 o01 = uv + float2(-pxSz.x, 0.0f);
	float2 o02 = uv + float2(-pxSz.x, pxSz.y);
	float2 o12 = uv + float2(0.0f, pxSz.y);
	float2 o21 = uv + float2(pxSz.x, 0.0f);
	float2 o22 = uv + float2(pxSz.x, pxSz.y);

	// 소벨 필터를 적용하려면 현재 필셀 주위의 여덟 픽셀이 필요한다.
	float h00 = GetHeightMapSample(o00);
	float h10 = GetHeightMapSample(o10);
	float h20 = GetHeightMapSample(o20);
	float h01 = GetHeightMapSample(o01);
	float h02 = GetHeightMapSample(o02);
	float h12 = GetHeightMapSample(o12);
	float h21 = GetHeightMapSample(o21);
	float h22 = GetHeightMapSample(o22);

	// 소벨 필터들을 평가한다.
	float gx = h00 - h20 + 2.0f * h01 - 2.0f * h21 + h02 - h22;
	float gy = h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;

	// 빠진 z 성분을 생성한다.
	float gz = 0.01f * sqrt(max(0.0f, 1.0f - gx * gx - gy * gy));

	return normalize(float3(2.0f * gx, gz, 2.0f * gy));
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

	// 각 타일의 적절한 LOD를 결정한다.
	float dist[] =
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

	// 현재 타일 내부 조각의 LOD는 항상 타일 자체의 LOD와 같다.
	pt.mInsideTess[0] = pt.mInsideTess[1] = dist[0];

	// 이웃 타일의 LOD가 더 낮다면 그 LOD를 가장자리 조각의 LOD로 설정한다.
	// 이웃 타일의 LOD가 더 높다면 가장자리 조각에 그대로 사용한다.
	// (그 이웃 타일이 현재 타일에 맞게 자신의 가장자리 LOD를 낮춘다.)
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

	finalVertexCoord.y += GetHeightMapSample(texCoord);

	// 세계 공간에서의 좌표를 전달한다.
	dout.mPosW = finalVertexCoord;
	// 동차 절단 공간으로 변환한다.
	dout.mPosH = mul(float4(finalVertexCoord, 1.0f), gViewProj);
	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	dout.mTexC = mul(float4(texCoord, 0.0f, 1.0f), gMaterialData[gMaterialIndex].mMatTransform).xy;
	// 소벨 연산자를 사용하여 노멀을 계산한다.
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

	// 조명되는 픽셀에서 눈으로의 벡터
	float3 toEyeW = gEyePosW - pin.mPosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse를 전반적으로 밝혀주는 Ambient항
	float4 ambient = gAmbientLight * diffuse;

	// 노멀을 정규화한다.
	pin.mNormal = normalize(pin.mNormal);

	// roughness와 normal를 이용하여 shininess를 계산한다.
	const float shininess = 1.0f - roughness;
	Material mat = { diffuse, specular, shininess };

	// Terrain에선 gLight의 DirectionalLight만을 라이팅한다.
	float4 directLight = ComputeOnlyDirectionalLight(gLights, mat, pin.mNormal, toEyeW);
	float4 litColor = ambient + directLight;

	if (gFogEnabled)
	{
		litColor = GetFogBlend(litColor, distToEye);
	}

	return litColor;
}