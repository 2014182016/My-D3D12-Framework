#include "pch.h"
#include "Particle.h"

using namespace DirectX;

Particle::Particle(std::string&& name,UINT maxParticleNum) : Object(std::move(name)) 
{
	mMaxParticleNum = maxParticleNum;
	particleArray = new ParticleType[mMaxParticleNum];
}

Particle::~Particle() 
{
	delete[] particleArray;
}

void Particle::Tick(float deltaTime)
{
	if (mIsActive)
	{
		if (mSpawnTime < 0.0f)
		{
			SpawnParticleType();
;			mSpawnTime = GetRandomFloat(mSpawnTimeRange.first, mSpawnTimeRange.second);
		}

		mSpawnTime -= deltaTime;
		DestroyParticleType();
	}
}

void Particle::SpawnParticleType()
{
	if (mActiveParticleNum <= mMaxParticleNum)
		return;

	ParticleType particleType;

	particleType.mPosition = GetRandomFloat3(mSpawnPosRange.first, mSpawnPosRange.second);
	particleType.mVelocity = GetRandomFloat3(mVelocityRange.first, mVelocityRange.second);
	particleType.mSize = GetRandomFloat(mSizeRange.first, mSizeRange.second);
	particleType.mLifeTime = GetRandomFloat(mLifeTimeRange.first, mLifeTimeRange.second);

	XMFLOAT3 colorDiffuse = GetRandomFloat3(mSpawnPosRange.first, mSpawnPosRange.second);
	float opacity = GetRandomFloat(mOpacity.first, mOpacity.second);
	particleType.mColor = XMFLOAT4(colorDiffuse.x, colorDiffuse.y, colorDiffuse.z, opacity);

	if (mFacingCamera)
		particleType.mNormal = GetRandomNormal();

	particleArray[mActiveParticleNum++] = particleType;
}

void Particle::DestroyParticleType()
{
	for (int i = 0; i < mActiveParticleNum; ++i)
	{
		if (particleArray[i].mLifeTime < 0.0f)
		{
			
		}
	}
}