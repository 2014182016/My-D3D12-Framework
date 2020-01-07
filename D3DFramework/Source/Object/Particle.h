#pragma once

#include "Object.h"
#include "ObjectPool.hpp"
#include "../Framework/Interfaces.h"

#define GA -9.8f

class Particle : public Object, public Renderable
{
private:
	struct ParticleData
	{
		DirectX::XMFLOAT3 mPosition;
		DirectX::XMFLOAT4 mColor;
		DirectX::XMFLOAT3 mVelocity;
		DirectX::XMFLOAT3 mNormal;
		DirectX::XMFLOAT2 mSize;
		float mLifeTime;
	};

public:
	Particle(std::string&& name, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT maxParticleNum);
	virtual ~Particle();

public:
	virtual void Tick(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* commandList);

public:
	inline UINT GetMaxParticleNum() const { return mMaxParticleNum; }

	inline void SetSpawnDistanceRange(DirectX::XMFLOAT3 posMin, DirectX::XMFLOAT3 posMax) { mSpawnDistanceRange.first = posMin; mSpawnDistanceRange.second = posMax; }
	inline void SetVelocityRange(DirectX::XMFLOAT3 velMin, DirectX::XMFLOAT3 velMax) { mVelocityRange.first = velMin; mVelocityRange.second = velMax; }
	inline void SetColorRange(DirectX::XMFLOAT4 colorMin, DirectX::XMFLOAT4 colorMax) { mColorRange.first = colorMin; mColorRange.second = colorMax; }
	inline void SetSizeRange(DirectX::XMFLOAT2 sizeMin, DirectX::XMFLOAT2 sizeMax) { mSizeRange.first = sizeMin; mSizeRange.second = sizeMax; }
	inline void SetSpawnTimeRange(float timeMin, float timeMax) { mSpawnTimeRange.first = timeMin; mSpawnTimeRange.second = timeMax; }
	inline void SetLifeTimeRange(float timeMin, float timeMax) { mLifeTimeRange.first = timeMin; mLifeTimeRange.second = timeMax; }

	inline void SetSpawnDistanceRange(DirectX::XMFLOAT3 pos) { mSpawnDistanceRange.first = pos; mSpawnDistanceRange.second = pos; }
	inline void SetVelocityRange(DirectX::XMFLOAT3 vel) { mVelocityRange.first = vel; mVelocityRange.second = vel; }
	inline void SetColorRange(DirectX::XMFLOAT4 color) { mColorRange.first = color; mColorRange.second = color; }
	inline void SetSizeRange(DirectX::XMFLOAT2 size) { mSizeRange.first = size; mSizeRange.second = size; }
	inline void SetSpawnTimeRange(float time) { mSpawnTimeRange.first = time; mSpawnTimeRange.second = time; }
	inline void SetLifeTimeRange(float time) { mLifeTimeRange.first = time; mLifeTimeRange.second = time; }

	inline void SetLifeTime(float time) { mLifeTime = time; }
	inline void SetBurstNum(UINT num) { mBurstNum = num; }

	inline void SetActive(bool value) { mIsActive = value; }
	inline bool GetIsActive() const { return mIsActive; }

	inline void SetFacingCamera(bool value) { mFacingCamera = value; }
	inline void SetEnabledGravity(bool value) { mEnabledGravity = value; }
	inline void SetEnabledInfinite(bool value) { mIsInfinite = value; }
	inline void SetInfinite(bool value) { mIsInfinite = value; }
	inline bool GetIsFacingCamera() const { return mFacingCamera; }
	inline float GetLifeTime() const { return mLifeTime; }

	inline const std::list<ParticleData*>& GetParticleDatas() const { return mParticleDatas; }

private:
	void SpawnParticleData();
	void DestroyParticleData();
	void UpdateParticleData(float deltaTime);

protected:
	UINT mMaxParticleNum = 0;

	std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> mSpawnDistanceRange;
	std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> mVelocityRange;
	std::pair<DirectX::XMFLOAT4, DirectX::XMFLOAT4> mColorRange;
	std::pair<DirectX::XMFLOAT2, DirectX::XMFLOAT2> mSizeRange;
	std::pair<float, float> mLifeTimeRange;

	std::pair<float, float> mSpawnTimeRange;
	float mSpawnTime = 0.0f;
	float mLifeTime = 0.0f;
	UINT mBurstNum = 1;

private:
	std::unique_ptr<ObjectPool<ParticleData>> mParitlcePool;
	std::list<ParticleData*> mParticleDatas;
	std::unique_ptr<class Mesh> mParticleMesh;

	bool mIsActive = true;
	bool mEnabledGravity = false;
	bool mFacingCamera = true;
	bool mIsInfinite = false;
};