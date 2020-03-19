#include "ParticleRS.hlsl"

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

		uint index = gCounter[1] + myID;
		if (gMaxParticleNum <= index)
		{
			index -= gMaxParticleNum;
		}

		float3 randVec = float3(rand(float2(myID, index)), rand(float2(index, myID)), rand(float2(myID - index, index - myID)));
		p.mDirection = normalize(reflect(direction[gropuThreadID.x], randVec));
		p.mPosition = gEmitterLocation;
		p.mLifeTime = gStart.mLifeTime;
		p.mColor = gStart.mColor;
		p.mSpeed = gStart.mSpeed;
		p.mSize = gStart.mSize;
		p.mIsActive = true;

		InterlockedAdd(gCounter[0], 1);

		gParticles[index] = p;
	}

	GroupMemoryBarrierWithGroupSync();

	if (myID == 0)
	{
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
		if (gParticles[myID].mIsActive == true)
		{
			if (gParticles[myID].mLifeTime < 0.0f)
			{
				gParticles[myID].mIsActive = false;
				InterlockedAdd(gCounter[0], -1);
			}
			else
			{
				gParticles[myID].mLifeTime -= gDeltaTime;

				float period = saturate(gParticles[myID].mLifeTime / gStart.mLifeTime);
				gParticles[myID].mColor = lerp(gEnd.mColor, gStart.mColor, period);
				gParticles[myID].mSpeed = lerp(gEnd.mSpeed, gStart.mSpeed, period);
				gParticles[myID].mSize = lerp(gEnd.mSize, gStart.mSize, period);
				gParticles[myID].mPosition += gParticles[myID].mDirection * gParticles[myID].mSpeed * gDeltaTime;

				if (gEnabledGravity == 1)
				{
					gParticles[myID].mPosition.y -= GA * gDeltaTime;
				}
			}
		}
	}
}