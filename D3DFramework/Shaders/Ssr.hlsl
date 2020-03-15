#include "Global.hlsl"

Texture2D gPositionMap : register(t0);
Texture2D gNormalMap : register(t1);
Texture2D gColorMap : register(t2);

SamplerState gsamLinearClamp : register(s0);

cbuffer cbSsr : register(b0)
{
	float gMaxDistance; 
	float gThickness;
	int gRayTraceStep;
	int gBinaryStep;
	float2 gScreenEdgeFadeStart;
	float gStrideCutoff;
	float gResolutuon;
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

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

float4 GetViewPosition(float2 uv)
{
	float4 pos = gPositionMap.Sample(gsamLinearClamp, uv, 0.0f);
	return mul(pos, gView);
}

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float2 mTexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.mTexC = gTexCoords[vid];

	// ��ũ�� ��ǥ�迡�� ���� ��ǥ��� ��ȯ�Ѵ�.
	vout.mPosH = float4(2.0f * vout.mTexC.x - 1.0f, 1.0f - 2.0f * vout.mTexC.y, 0.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	uint2 texcoord = pin.mPosH.xy;
	float4 currPosV = gPositionMap.Load(uint3(texcoord, 0));

	// Position�� ���� ���� SSAO �� SSR üũ ���θ� �ٷ��.
	if (currPosV.a <= 0.0f)
		return 0.0f;

		   currPosV = mul(float4(currPosV.xyz, 1.0f), gView);
	float3 toPosition = normalize(currPosV.xyz);
	float3 normal = gNormalMap.Load(int3(texcoord, 0)).xyz;
		   normal = mul(normal, (float3x3)gView);
	float3 rayDirection = normalize(reflect(toPosition, normal));
	float rayLength = ((currPosV.z + rayDirection.z * gMaxDistance) < gNearZ) ?
		(gNearZ - currPosV.z) / rayDirection.z : gMaxDistance;
		  rayLength *= min(1.0f, currPosV.z / gStrideCutoff);

	float4 startPosV = float4(currPosV.xyz, 1.0f);
	float4 endPosV = float4(currPosV.xyz + rayDirection * rayLength, 1.0f);

	float4 startPosH = mul(startPosV, gProjTex);
		   startPosH /= startPosH.w;
		   startPosH.xy *= gRenderTargetSize;
	float4 endPosH = mul(endPosV, gProjTex);
		   endPosH /= endPosH.w;
		   endPosH.xy *= gRenderTargetSize;

	float deltaX = endPosH.x - startPosH.x;
	float deltaY = endPosH.y - startPosH.y;
	float useX = abs(deltaX) >= abs(deltaY) ? 1.0f : 0.0f;
	float delta = lerp(abs(deltaY), abs(deltaX), useX) / (float)gRayTraceStep;
	float2 increment = float2(deltaX, deltaY) / max(delta, 0.01f);

	float p0 = 0.0f;
	float p1 = 0.0f;

	int hitRayTracePass = 0;
	int hitBinaryPass = 0;

	float depth = 0.0f;
	float2 currPosH = startPosH.xy;
	float2 uv = currPosH * gInvRenderTargetSize;

	float jitter = (float)((texcoord.x + texcoord.y) & 1) * 0.5f;
	currPosH += increment * jitter;

	//////////////////////////////////////// Ray Trace //////////////////////////////////////

	[loop]
	for (int i = 0; i < gRayTraceStep; ++i)
	{
		currPosH += increment;
		uv = currPosH * gInvRenderTargetSize;

		currPosV = GetViewPosition(uv);

		p1 = saturate(lerp((currPosH.y - startPosH.y) / deltaY, (currPosH.x - startPosH.x) / deltaX, useX));

		float viewDistance = (startPosV.z * endPosV.z) / lerp(endPosV.z, startPosV.z, p1);
		depth = viewDistance - currPosV.z;

		if (depth > 0.0f && depth < gThickness)
		{
			hitRayTracePass = 1;
			break;
		}
		else
		{
			p0 = p1;
		}
	}

	float z0 = p0;
	float z1 = p1;
	p1 = p0 + ((z1 - z0) / 2);
	int secondSteps = gBinaryStep * hitRayTracePass;

	//////////////////////////////////////// Binary Step //////////////////////////////////////

	[loop]
	for (int j = 0; j < secondSteps; ++j)
	{
		currPosH = lerp(startPosH.xy, endPosH.xy, p1);
		uv = currPosH * gInvRenderTargetSize;

		currPosV = GetViewPosition(uv);

		float viewDistance = (startPosV.z * endPosV.z) / lerp(endPosV.z, startPosV.z, p1);
		depth = viewDistance - currPosV.z;

		if (depth > 0.0f && depth < gThickness)
		{
			hitBinaryPass = 1;
			break;
		}
		else if(depth <= 0.0f)
		{
			p1 = p1 + ((p1 - z0) / 2);
		}
		else
		{
			p1 = p1 + ((p1 - z1) / 2);
		}
	}

	if (hitBinaryPass == 0)
		return 0.0f;

	// �ش� ������������ ���� ���Ϳ� �ݻ�� �����Ͱ� ���� ��������
	// ��ü �޺κп� �浹���� ���ɼ��� ����. ���� ���̵� �ƿ���Ų��.
	float fade = 1.0f - max(dot(-toPosition, rayDirection), 0.0f);

	// �浹�� ã�� ��ġ�� ��¥ ��Ȯ�� ��ġ�� �ٸ� ���ɼ��� ����.
	// ���� gThickness���� ���̰� Ŭ���� ���̵� �ƿ���Ų��.
	fade *= 1.0f - saturate(depth / gThickness);

	// ã�� ��ġ�� �Ÿ��� �ּ��� ���̵� �ƿ���Ų��.
	fade *= saturate(length(currPosV.xyz - endPosV.xyz) / rayLength);

	// ȭ���� �����ڸ��� �������� EndPoint�� ȭ�� ������ ���� ������ ����
	// ���� �ʿ� ������ ���ɼ��� ũ��. ���� ã�� ��ġ�� ȭ�� �����ڸ��� 
	// �������� ���̵� �ƿ���Ų��.
	float2 minDimension = float2(min(currPosH.x, gRenderTargetSize.x - currPosH.x), 
		min(currPosH.y, gRenderTargetSize.y - currPosH.y));
	minDimension = clamp(minDimension, 0.0f, gScreenEdgeFadeStart) / gScreenEdgeFadeStart;
	fade *= min(minDimension.x, minDimension.y);

	// uv�� ������ ����ٸ� ���� ������� �ʴ´�.
	fade *= uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f ? 0.0f : 1.0f;

	float3 color = gColorMap.Sample(gsamLinearClamp, uv, 0.0f).rgb;
		   //color *= fade;

	return float4(color, fade);
}

