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

		// ���� ������ ��ƼŬ �ε����� ����Ѵ�.
		uint index = gCounter[1] + myID;
		if (gMaxParticleNum <= index)
		{
			index -= gMaxParticleNum;
		}

		// ��ƼŬ�� ������ �����ϰ� �����Ѵ�.
		float3 randVec = float3(rand(float2(myID, index)), rand(float2(index, myID)), rand(float2(myID - index, index - myID)));
		p.direction = normalize(reflect(direction[gropuThreadID.x], randVec));

		// ��ƼŬ�� ���� ��ġ�� ��ƼŬ ��ü�� �߽� ��ġ�̴�.
		p.position = gEmitterLocation;

		// start �����͸� �̿��Ͽ� ��ƼŬ �����͸� �����Ѵ�.
		p.lifeTime = gStart.lifeTime;
		p.color = gStart.color;
		p.speed = gStart.speed;
		p.size = gStart.size;

		// �ش� ��ƼŬ �����͸� Ȱ��ȭ�Ѵ�.
		p.isActive = true;

		// ����ȭ �Լ��� ����Ͽ� ī���͸� ������Ų��.
		InterlockedAdd(gCounter[0], 1);

		gParticles[index] = p;
	}

	// �׷� �����尡 ���⿡ ������ ������ ��ٸ���.
	GroupMemoryBarrierWithGroupSync();

	if (myID == 0)
	{
		// ���� ������ ��ƼŬ �ε����� ����Ѵ�.
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
				// ������ �� �� ��ƼŬ�� ��Ȱ��ȭ�Ѵ�.
				gParticles[myID].isActive = false;
				InterlockedAdd(gCounter[0], -1);
			}
			else
			{
				// ���� �ð���ŭ ������ ���δ�.
				gParticles[myID].lifeTime -= gDeltaTime;

				// ��ƼŬ �����͸� �ð��� ���� ������Ʈ�Ѵ�.
				float period = saturate(gParticles[myID].lifeTime / gStart.lifeTime);
				gParticles[myID].color = lerp(gEnd.color, gStart.color, period);
				gParticles[myID].speed = lerp(gEnd.speed, gStart.speed, period);
				gParticles[myID].size = lerp(gEnd.size, gStart.size, period);
				gParticles[myID].position += gParticles[myID].direction * gParticles[myID].speed * gDeltaTime;

				if (gEnabledGravity == 1)
				{
					// ��ƼŬ �����Ϳ� �߷��� �����Ѵ�.
					gParticles[myID].position.y -= GA * gDeltaTime;
				}
			}
		}
	}
}