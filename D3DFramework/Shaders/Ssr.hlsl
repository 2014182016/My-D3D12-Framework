#include "Pass.hlsl"
#include "Fullscreen.hlsl"

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

float4 GetViewPosition(float2 uv)
{
	// 월드 공간의 좌표를 뷰 공간으로 변환한다.
	float4 pos = gPositionMap.Sample(gsamLinearClamp, uv, 0.0f);
	return mul(pos, gView);
}

float4 PS(VertexOut pin) : SV_Target
{
	uint2 texcoord = pin.posH.xy;
	float4 currPosV = gPositionMap.Sample(gsamLinearClamp, pin.texC, 0.0f);

	// Position의 알파 값은 SSAO 및 SSR 체크 여부를 다룬다.
	if (currPosV.a <= 0.0f)
		return 0.0f;
	
	// 현재 픽셀의 월드 좌표계 위치 및 노멀을 뷰 공간으로 변환한다.
		   currPosV = mul(float4(currPosV.xyz, 1.0f), gView);
	float3 toPosition = normalize(currPosV.xyz);
	float3 normal = gNormalMap.Sample(gsamLinearClamp, pin.texC, 0.0f).xyz;
		   normal = mul(normal, (float3x3)gView);

	// 진행할 광선의 방향을 계산한다.
	float3 rayDirection = normalize(reflect(toPosition, normal));
	// 광선이 뷰 프러스텀보다 앞으로 오지 않게 제한한다.
	float rayLength = ((currPosV.z + rayDirection.z * gMaxDistance) < gNearZ) ?
		(gNearZ - currPosV.z) / rayDirection.z : gMaxDistance;
	// 카메라와 픽셀사이의 거리가 가깝다면 광선의 길이를 줄인다.
	//	  rayLength *= max(0.01f, currPosV.z / gStrideCutoff);

	// 시작과 끝지점의 뷰 공간 좌표를 계산한다.
	float4 startPosV = float4(currPosV.xyz, 1.0f);
	float4 endPosV = float4(currPosV.xyz + rayDirection * rayLength, 1.0f);

	// 시작과 끝지점의 동차 공간 좌표를 계산한다.
	float4 startPosH = mul(startPosV, gProjTex);
		   startPosH /= startPosH.w;
		   startPosH.xy *= gRenderTargetSize;
	float4 endPosH = mul(endPosV, gProjTex);
		   endPosH /= endPosH.w;
		   endPosH.xy *= gRenderTargetSize;

	// 진행할 광선의 변화율을 계산한다.
	float deltaX = endPosH.x - startPosH.x;
	float deltaY = endPosH.y - startPosH.y;
	float useX = abs(deltaX) >= abs(deltaY) ? 1.0f : 0.0f;
	float delta = lerp(abs(deltaY), abs(deltaX), useX) / (float)gRayTraceStep;
	float2 increment = float2(deltaX, deltaY) / max(delta, 0.01f);

	// 시작과 끝지점의 위치를 기록할 [0, 1]사이의 임시 변수
	float p0 = 0.0f;
	float p1 = 0.0f;

	// 각 패스마다 통과하였는지 여부를 나타낸다.
	int hitRayTracePass = 0;
	int hitBinaryPass = 0;

	// 찾아낸 위치의 정보
	float depth = 0.0f; // 광선과 현재 위치의 두께
	float2 currPosH = startPosH.xy;
	float2 uv = currPosH * gInvRenderTargetSize;

	// 레이 마칭의 단점으로 진행되는 광선이 계단현상으로 나타나는 아티팩트가 생긴다.
	// 따라서 시작지점을 랜덤한 값으로 이동시켜 아티팩트를 줄인다.
	float jitter = (float)((texcoord.x + texcoord.y) & 1) * 0.5f;
	currPosH += increment * jitter;

	//////////////////////////////////////// Ray Trace //////////////////////////////////////

	[loop]
	for (int i = 0; i < gRayTraceStep; ++i)
	{
		// 다음 위치를 계산한다.
		currPosH += increment;
		uv = currPosH * gInvRenderTargetSize;

		// 뷰 공간 좌표를 가져온다.
		currPosV = GetViewPosition(uv);

		// 현재 지점을 시작과 끝지점의 [0, 1]사이로 변환한다.
		p1 = saturate(lerp((currPosH.y - startPosH.y) / deltaY, (currPosH.x - startPosH.x) / deltaX, useX));

		// 현재 진행된 위치의 깊이를 계산하고, depth를 계산한다.
		float viewDistance = (startPosV.z * endPosV.z) / lerp(endPosV.z, startPosV.z, p1);
		depth = viewDistance - currPosV.z;

		// depth가 0이상이라면 현재 위치가 보이고, 
		// 두께가 일정 이하이면 조건을 만족한다.
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
		// 다음 위치를 계산한다.
		currPosH = lerp(startPosH.xy, endPosH.xy, p1);
		uv = currPosH * gInvRenderTargetSize;

		// 뷰 공간 좌표를 가져온다.
		currPosV = GetViewPosition(uv);

		// 현재 진행된 위치의 깊이를 계산하고, depth를 계산한다.
		float viewDistance = (startPosV.z * endPosV.z) / lerp(endPosV.z, startPosV.z, p1);
		depth = viewDistance - currPosV.z;

		// depth가 0이상이라면 현재 위치가 보이고, 
		// 두께가 일정 이하이면 조건을 만족한다.
		if (depth > 0.0f && depth < gThickness)
		{
			hitBinaryPass = 1;
			break;
		}
		else if(depth <= 0.0f)
		{
			// 두께가 0이하라면 뒷방향으로 진행한다.
			p1 = p1 + ((p1 - z0) / 2);
		}
		else
		{
			// 두께가 0이상이라면 앞방향으로 진행한다.
			p1 = p1 + ((p1 - z1) / 2);
		}
	}

	if (hitBinaryPass == 0)
		return 0.0f;

	// 해당 점에서부터의 방향 벡터와 반사된 빛벡터가 거의 같을수록
	// 물체 뒷부분에 충돌했을 가능성이 높다. 따라서 페이드 아웃시킨다.
	float fade = 1.0f - max(dot(-toPosition, rayDirection), 0.0f);

	// 충돌을 찾은 위치와 진짜 정확한 위치는 다를 가능성이 높다.
	// 따라서 gThickness와의 차이가 클수록 페이드 아웃시킨다.
	fade *= 1.0f - saturate(depth / gThickness);

	// 찾은 위치와 거리가 멀수록 페이드 아웃시킨다.
	fade *= saturate(length(currPosV.xyz - endPosV.xyz) / rayLength);

	// 화면의 가장자리에 있을수록 EndPoint가 화면 밖으로 나가 정보가 없는
	// 버퍼 맵에 접근할 가능성이 크다. 따라서 찾은 위치가 화면 가장자리에 
	// 있을수록 페이드 아웃시킨다.
	float2 minDimension = float2(min(currPosH.x, gRenderTargetSize.x - currPosH.x), 
		min(currPosH.y, gRenderTargetSize.y - currPosH.y));
	minDimension = clamp(minDimension, 0.0f, gScreenEdgeFadeStart) / gScreenEdgeFadeStart;
	fade *= min(minDimension.x, minDimension.y);

	// uv가 범위를 벗어났다면 값을 기록하지 않는다.
	fade *= uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f ? 0.0f : 1.0f;

	float3 color = gColorMap.Sample(gsamLinearClamp, uv, 0.0f).rgb;
		   //color *= fade;

	// 반사된 값을 기록한다.
	return float4(color, fade);
}

