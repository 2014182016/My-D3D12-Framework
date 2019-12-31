#include "pch.h"
#include "Particle.h"
#include "MeshGeometry.h"
#include "Structures.h"

using namespace std::literals;
using namespace DirectX;

Particle::Particle(std::string&& name, UINT maxParticleNum) : Object(std::move(name)) 
{
	mMaxParticleNum = maxParticleNum;
	mParitlcePool = std::make_unique<ObjectPool<ParticleData>>(maxParticleNum);
}

Particle::~Particle() { }

void Particle::Tick(float deltaTime)
{
	mLifeTime -= deltaTime;
	if (mLifeTime < 0.0f)
		Destroy();

	if (mIsActive)
	{
		mSpawnTime -= deltaTime;
		if (mSpawnTime < 0.0f)
		{
			for (UINT i = 0; i < mBurstNum; ++i)
				SpawnParticleData();
;			mSpawnTime = GetRandomFloat(mSpawnTimeRange.first, mSpawnTimeRange.second);
		}

		UpdateParticleData(deltaTime);
		DestroyParticleData();
	}
}


void Particle::SpawnParticleData()
{
	if (mParitlcePool->Size() <= 0)
		return;

	ParticleData* particleData = mParitlcePool->Acquire();

	particleData->mPosition = mPosition + GetRandomFloat3(mSpawnDistanceRange.first, mSpawnDistanceRange.second);
	particleData->mVelocity = GetRandomFloat3(mVelocityRange.first, mVelocityRange.second);
	particleData->mSize = GetRandomFloat2(mSizeRange.first, mSizeRange.second);
	particleData->mLifeTime = GetRandomFloat(mLifeTimeRange.first, mLifeTimeRange.second);
	particleData->mColor = GetRandomFloat4(mColorRange.first, mColorRange.second);

	if (mFacingCamera)
		particleData->mNormal = GetRandomNormal();

	mParticleDatas.push_back(particleData);
}

void Particle::DestroyParticleData()
{
	for (auto iter = mParticleDatas.begin(); iter != mParticleDatas.end(); )
	{
		ParticleData* data = *iter;
		if (data->mLifeTime < 0.0f)
		{
			iter = mParticleDatas.erase(iter);
			mParitlcePool->Release(data);
			continue;
		}
		else
		{
			++iter;
		}
	}
}

void Particle::UpdateParticleData(float deltaTime)
{
	for (auto& particleData : mParticleDatas)
	{
		if (mEnabledGravity)
		{
			particleData->mVelocity.y += GA * deltaTime;
			particleData->mPosition = particleData->mPosition + particleData->mVelocity * deltaTime;
		}
		else
		{
			particleData->mPosition = particleData->mPosition + particleData->mVelocity * deltaTime;
		}
		particleData->mLifeTime -= deltaTime;
	}

	UpdateNumFrames();
}

void Particle::BuildParticleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mMesh = std::make_unique<MeshGeometry>(GetName() + "Mesh"s + std::to_string(mParticleIndex));
	mMesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}