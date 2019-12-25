#pragma once

#include "Object.h"

class Particle : public Object
{
private:
	struct ParticleType
	{
		DirectX::XMFLOAT3 mPosition;
		DirectX::XMFLOAT4 mColor;
		DirectX::XMFLOAT3 mVelocity;
		DirectX::XMFLOAT3 mNormal;
		float mSize;
		float mLifeTime;
	};

public:
	Particle(std::string&& name, UINT maxParticleNum);
	virtual ~Particle();

public:
	virtual void Tick(float deltaTime) override;

public:
	inline UINT GetMaxParticleNum() const { return mMaxParticleNum; }

	inline void SetSpawnPosRange(DirectX::XMFLOAT3 posMin, DirectX::XMFLOAT3 posMax) { mSpawnPosRange.first = posMin; mSpawnPosRange.second = posMax; }
	inline void SetVelocityRange(DirectX::XMFLOAT3 velMin, DirectX::XMFLOAT3 velMax) { mVelocityRange.first = velMin; mVelocityRange.second = velMax; }
	inline void SetColorRange(DirectX::XMFLOAT4 colorMin, DirectX::XMFLOAT4 colorMax) { mColorRange.first = colorMin; mColorRange.second = colorMax; }
	inline void SetSizeRange(float sizeMin, float sizeMax) { mSizeRange.first = sizeMin; mSizeRange.second = sizeMax; }
	inline void SetSpawnTimeRange(float timeMin, float timeMax) { mSpawnTimeRange.first = timeMin; mSpawnTimeRange.second = timeMax; }
	inline void SetLifeTimeRange(float timeMin, float timeMax) { mLifeTimeRange.first = timeMin; mLifeTimeRange.second = timeMax; }
	inline void SetOpacityRange(float opacityMin, float opacityMax) { mOpacity.first = opacityMin; mOpacity.second = opacityMax; }

	inline void SetSpawnPosRange(DirectX::XMFLOAT3 pos) { mSpawnPosRange.first = pos; mSpawnPosRange.second = pos; }
	inline void SetVelocityRange(DirectX::XMFLOAT3 vel) { mVelocityRange.first = vel; mVelocityRange.second = vel; }
	inline void SetColorRange(DirectX::XMFLOAT4 color) { mColorRange.first = color; mColorRange.second = color; }
	inline void SetSizeRange(float size) { mSizeRange.first = size; mSizeRange.second = size; }
	inline void SetSpawnTimeRange(float time) { mSpawnTimeRange.first = time; mSpawnTimeRange.second = time; }
	inline void SetLifeTimeRange(float time) { mLifeTimeRange.first = time; mLifeTimeRange.second = time; }
	inline void SetOpacityRange(float opacity) { mOpacity.first = opacity; mOpacity.second = opacity; }

	inline void SetActive(bool value) { mIsActive = value; }
	inline bool GetIsActive() const { return mIsActive; }

	inline void SetEnabledLighting(bool value) { mEnabledLighting = value; }
	inline void SetFacingCamera(bool value) { mFacingCamera = value; }

	inline void SetMaterial(class Material* material) { mMaterial = material; }
	inline class Material* GetMaterial() { return mMaterial; }

private:
	void SpawnParticleType();
	void DestroyParticleType();

protected:
	ParticleType* particleArray = nullptr;

	UINT mMaxParticleNum = 0;
	UINT mActiveParticleNum = 0;

	std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> mSpawnPosRange;
	std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> mVelocityRange;
	std::pair<DirectX::XMFLOAT4, DirectX::XMFLOAT4> mColorRange;
	std::pair<float, float> mOpacity;
	std::pair<float, float> mSizeRange;
	std::pair<float, float> mLifeTimeRange;

	std::pair<float, float> mSpawnTimeRange;
	float mSpawnTime = 0.0f;

private:
	bool mIsActive = true;
	bool mEnabledLighting = false;
	bool mFacingCamera = true;
	class Material* mMaterial = nullptr;
};