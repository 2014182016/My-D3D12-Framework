#include "Global.hlsl"
#include "ParticleRS.hlsl"

const static float2 texC[4] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 1.0f),
	float2(1.0f, 0.0f)
};

StructuredBuffer<ParticleData>  particles : register(t0);
Texture2D gTextureMaps[TEX_NUM] : register(t0, space1);

SamplerState gsamLinearWrap : register(s0);

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
	float gpDeltaTime;
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

struct VertexOut
{
	float3 mPosW   : POSITION;
	float4 mColor  : COLOR;
	float2 mSize   : SIZE;
	bool mIsActive : ACTIVE;
};

struct GeoOut
{
	float4 mPosH   : SV_POSITION;
	float3 mPosW   : POSITION;
	float3 mNormal : NORMAL;
	float4 mColor  : COLOR;
	float2 mTexC   : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout = (VertexOut)0.0f;

	if (particles[vid].mIsActive == true)
	{
		vout.mPosW = particles[vid].mPosition;
		vout.mColor = particles[vid].mColor;
		vout.mSize = particles[vid].mSize;
		vout.mIsActive = true;
	}

	return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1],
	inout TriangleStream<GeoOut> triStream)
{
	if (gin[0].mIsActive == false)
		return;

	// 빌보드가 xz 평면에 붙어서 y 방향으로 세워진 상태에서 카메라를
	// 향하게 만드는 세계 공간 기준 빌보드 지역 좌표계를 계산한다.
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].mPosW;
	look.y = 0.0f; // y 축 정렬이므로 sz평면에 투영
	look = normalize(look);
	float3 right = cross(up, look);

	// 세계 공간 기준의 삼각형 띠 정점들(사각형을 구성하는)을 계산한다.
	float halfWidth = 0.5f * gin[0].mSize.x;
	float halfHeight = 0.5f * gin[0].mSize.y;

	float4 v[4];
	v[0] = float4(gin[0].mPosW + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin[0].mPosW + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin[0].mPosW - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin[0].mPosW - halfWidth * right + halfHeight * up, 1.0f);

	GeoOut gout;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gout.mPosH = mul(v[i], gViewProj);
		gout.mPosW = v[i].xyz;
		gout.mNormal = look;
		gout.mTexC = texC[i];
		gout.mColor = gin[0].mColor;

		triStream.Append(gout);
	}

	triStream.RestartStrip();
}

float4 PS(GeoOut pin) : SV_Target
{
	float4 color = 0.0f;
	if (gTextureIndex != DISABLED)
	{
		color = gTextureMaps[gTextureIndex].Sample(gsamLinearWrap, pin.mTexC);
		color *= pin.mColor;
	}
	else
	{
		color = pin.mColor;
	}

	return color;
}