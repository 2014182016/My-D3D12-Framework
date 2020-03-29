#include "Particle.hlsl"

#define GA 9.8f

RWStructuredBuffer<uint> gCounter : register(u0);
RWStructuredBuffer<ParticleData> gParticles : register(u1);

float rand(in float2 uv)
{
	float2 noise = (frac(sin(dot(uv, float2(12.9898f, 78.233f)*2.0f)) * 43758.5453f));
	return frac(abs(noise.x + noise.y) * 0.5f);
}


#define NUM_EMIT_THREADS 8
[numthreads(NUM_EMIT_THREADS, 1, 1)]
void CS_Emit(uint3 gropuThreadID : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	uint myID = NUM_EMIT_THREADS * groupID.x + gropuThreadID.x;

	if (myID < gEmitNum)
	{
		ParticleData p = (ParticleData)0.0f;

		// 현재 생성할 파티클 인덱스를 계산한다.
		uint index = gCounter[1] + myID;
		if (gMaxParticleNum <= index)
		{
			index -= gMaxParticleNum;
		}

		// 파티클의 방향은 랜덤하게 조정한다.
		float3 randVec = float3(rand(float2(myID, index)), rand(float2(index, myID)), rand(float2(myID - index, index - myID)));
		p.direction = normalize(reflect(direction[gropuThreadID.x], randVec));

		// 파티클의 스폰 위치는 파티클 객체의 중심 위치이다.
		p.position = gEmitterLocation;

		// start 데이터를 이용하여 파티클 데이터를 설정한다.
		p.lifeTime = gStart.lifeTime;
		p.color = gStart.color;
		p.speed = gStart.speed;
		p.size = gStart.size;

		// 해당 파티클 데이터를 활성화한다.
		p.isActive = true;

		// 동기화 함수를 사용하여 카운터를 증가시킨다.
		InterlockedAdd(gCounter[0], 1);

		gParticles[index] = p;
	}

	// 그룹 스레드가 여기에 도달할 때까지 기다린다.
	GroupMemoryBarrierWithGroupSync();

	if (myID == 0)
	{
		// 새로 갱신할 파티클 인덱스를 계산한다.
		if (gMaxParticleNum <= gCounter[1] + gEmitNum)
		{
			gCounter[1] += gEmitNum - gMaxParticleNum;
		}
		else
		{
			gCounter[1] += gEmitNum;
		}
	}
}

#define NUM_UPDATE_THREADS 512
[numthreads(NUM_UPDATE_THREADS, 1, 1)]
void CS_Update(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint myID = dispatchThreadID.x;

	if (myID < gMaxParticleNum)
	{
		if (gParticles[myID].isActive == true)
		{
			if (gParticles[myID].lifeTime < 0.0f)
			{
				// 수명이 다 된 파티클은 비활성화한다.
				gParticles[myID].isActive = false;
				InterlockedAdd(gCounter[0], -1);
			}
			else
			{
				// 지난 시간만큼 수명을 줄인다.
				gParticles[myID].lifeTime -= gDeltaTime;

				// 파티클 데이터를 시간에 따라 업데이트한다.
				float period = saturate(gParticles[myID].lifeTime / gStart.lifeTime);
				gParticles[myID].color = lerp(gEnd.color, gStart.color, period);
				gParticles[myID].speed = lerp(gEnd.speed, gStart.speed, period);
				gParticles[myID].size = lerp(gEnd.size, gStart.size, period);
				gParticles[myID].position += gParticles[myID].direction * gParticles[myID].speed * gDeltaTime;

				if (gEnabledGravity == 1)
				{
					// 파티클 데이터에 중력을 적용한다.
					gParticles[myID].position.y -= GA * gDeltaTime;
				}
			}
		}
	}
}