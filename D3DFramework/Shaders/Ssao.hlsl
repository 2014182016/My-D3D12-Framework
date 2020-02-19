//=============================================================================
// Ssao.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//=============================================================================

//#include "SsaoInclude.hlsl"
#include "Common.hlsl"

struct VertexOut
{
	float4 mPosH : SV_POSITION;
	float3 mPosV : POSITION;
	float2 mTexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.mTexC = gTexCoords[vid];

	// 스크린 좌표계에서 동차 좌표계로 변환한다.
	vout.mPosH = float4(2.0f * vout.mTexC.x - 1.0f, 1.0f - 2.0f * vout.mTexC.y, 0.0f, 1.0f);
	
	float4 ph = mul(vout.mPosH, gInvProj);
	vout.mPosV = ph.xyz / ph.w;

	return vout;
}

float4 PS(VertexOut pin) :SV_Target
{
	// p : 현재 주변광 차폐를 계산하고자 하는 픽셀에 해당하는 점
	// n : p에서의 법선 벡터
	// q : p 주변의 무작위 점
	// r : p를 가릴 가능성이 있는 잠재적 차폐점

	int3 texcoord = int3(pin.mPosH.xy, 0);
	float3 normal = gNormalMap.Load(texcoord).rgb;
	float depth = gDepthMap.Load(texcoord).r;
	depth = NdcDepthToViewDepth(depth);

	// 완전한 시야공간 (x,y,z)를 재구축한다.
	// 우선 p = t * pin.mPosV를 만족하는 t를 구한다.
	// p.z = t * pin.mPosV.z
	// t = p.z / pin.mPosV.z
	float3 p = (depth / pin.mPosV.z) * pin.mPosV;

	// 무작위 벡터를 추출해서 [0, 1]을 [-1, 1]로 사상한다.
	float3 randVec = 2.0f * gRandomVecMap.SampleLevel(gsamLinearWrap, 4.0f * pin.mTexC, 0.0f).rgb - 1.0f;

	float occlusionSum = 0.0f;

	// n방향의 반구에서 p 주변의 이웃 표본점들을 추출한다.
	for (int i = 0; i < gSampleCount; ++i)
	{
		// 미리 만들어둔 상수 오프셋 벡터들은 고르게 분포되어 있다.
		// (즉, 오프셋 벡터들은 같은 방향으로 뭉쳐있지 않다.)
		// 한 무작위 벡터를 기준으로 이들을 반사시키면 고르게
		// 분포된 무작위 벡터들이 만들어진다.
		float3 offset = reflect(gOffsetVectors[i].xyz, randVec);

		// 오프셋 벡터가 (p, n)으로 정의되는 평면의 뒤쪽을 향하고 있으면
		// 방향을 반대로 뒤집는다.
		float flip = sign(dot(offset, normal));

		// p를 중심으로 차폐 반지름 이내의 무작위 점 q를 선택한다.
		float3 q = p + flip * gOcclusionRadius * offset;

		// q를 투영해서 투영 텍스처 좌표를 구한다.
		float4 projQ = mul(float4(q, 1.0f), gProjTex);
		projQ /= projQ.w;

		// 시점에서 q를 향한 반직선에서 가장 가까운 픽셀의 깊이를 구한다.
		// (이것이 q의 깊이는 아니다. q는 그냥 p 근처의 임의의 점이며)
		// 장면의 물체가 아닌 빈 공간에 있는 점일 수 있다.)
		// 가장 가까운 깊이는 깊이 맵에서 추출한다.
		float rz = gDepthMap.SampleLevel(gsamLinearWrap, projQ.xy, 0.0f).r;
		rz = NdcDepthToViewDepth(rz);

		// 완전한 시야 공간 위치 r = (rx, ry, rz)를 재구축한다.
		// r은 q를 지나는 반직선에 있으므로 r = t * q를 만족하는 t가 존재한다.
		// r.z = t * q.z이므로 t = r.z / q.z 이다.
		float3 r = (rz / q.z) * q;

		// r이 p를 가리는 지 판정한다.
		// * 내적 dot(normal, normalize(r - p))는 잠재적 차폐점 r이 (p, normal)으로
		//   정의되는 평면보다 얼마나 앞에 있는지를 나타낸다. 더 앞에 있는 점일수록
		//   차폐도의 가중치를 더 크게 잡는다. 이렇게 하면 자기 차폐문제, 즉 r이
		//   시선과 직각인 평면 (p, normal)에 있을 때 시점 시준의 깊이 값 차이때문에
		//   r이 p를 가린다고 잘못 판정하는 문제도 방지된다.
		// * 차폐도는 현재 점 p와 차폐점 r 사이의 거리에 의존한다. r이 p에서 너무 멀리
		//   있으면 p를 가리지 않는 것으로 판정한다.
		float distZ = p.z - r.z;
		float dp = max(dot(normal, normalize(r - p)), 0.0f);
		float occlusion = dp * OcclusionFunction(distZ);

		occlusionSum += occlusion;
	}

	occlusionSum /= gSampleCount;

	float access = 1.0f - occlusionSum;

	// SSAO가 좀 더 극적인 효과를 내도록, SSAO의 대비(constrast)를 증가한다.
	return saturate(pow(access, 2.0f));
}