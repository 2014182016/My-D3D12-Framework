#pragma once

#include "Object.h"
#include "../Framework/Structures.h"
#include "../Framework/Interfaces.h"

//#define BUFFER_COPY
#define NUM_EMIT_THREAD 8.0f
#define NUM_UPDATE_THREAD 512.0f

class Particle : public Object, public Renderable
{
public:
	Particle(std::string&& name, int maxParticleNum);
	virtual ~Particle();

public:
	virtual void Tick(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* commandList) override;

public:
	void CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, UINT cbvSrvUavDescriptorSize);
	void Update(ID3D12GraphicsCommandList* cmdList);
	void Emit(ID3D12GraphicsCommandList* cmdList);
	void SetBufferSrv(ID3D12GraphicsCommandList* cmdList);
	void SetBufferUav(ID3D12GraphicsCommandList* cmdList);
	void CopyData(ID3D12GraphicsCommandList* cmdList);

public:
	inline void SetParticleDataStart(ParticleData& data) { mStart = data; }
	inline void SetParticleDataEnd(ParticleData& data) { mEnd = data; }

	inline void SetSpawnTimeRange(float min, float max) { mSpawnTimeRange.first = min; mSpawnTimeRange.second = max; }
	inline void SetSpawnTimeRange(float time) { mSpawnTimeRange.first = time; mSpawnTimeRange.second = time; }
	inline float GetSpawnTime() const { return mSpawnTime; }

	inline void SetLifeTime(float time) { mLifeTime = time; }
	inline float GetLifeTime() const { return mLifeTime; }

	inline void SetActive(bool value) { mIsActive = value; }
	inline bool GetIsActive() const { return mIsActive; }

	inline void SetEmitNum(UINT num) { mEmitNum = num; }
	inline UINT GetEmitNum() const { return mEmitNum; }

	inline void SetEnabledGravity(bool value) { mEnabledGravity = value; }
	inline void SetEnabledInfinite(bool value) { mIsInfinite = value; }
	inline void SetInfinite(bool value) { mIsInfinite = value; }

	inline int GetMaxParticleNum() const { return mMaxParticleNum; }
	inline int GetCurrentParticleNum() const { return mCurrentParticleNum; }

	void SetParticleConstants(ParticleConstants& constants);

protected:
	int mMaxParticleNum = 0;
	int mCurrentParticleNum = 0;

	ParticleData mStart;
	ParticleData mEnd;

	std::pair<float, float> mSpawnTimeRange;
	float mSpawnTime = 0.0f;
	float mLifeTime = 0.0f;
	UINT mEmitNum = 1;

private:
	bool mIsActive = true;
	bool mEnabledGravity = false;
	bool mIsInfinite = false;

	Microsoft::WRL::ComPtr<ID3D12Resource> mBuffer;
#ifdef BUFFER_COPY
	Microsoft::WRL::ComPtr<ID3D12Resource> mReadBackBuffer;
#endif

	Microsoft::WRL::ComPtr<ID3D12Resource> mReadBackCounter;
	// 0번쨰 값은 현재 존재하는 파티클의 수, 1번쨰 값은 할당할 인덱스의 값
	Microsoft::WRL::ComPtr<ID3D12Resource> mBufferCounter;
};