#pragma once

#include "Object.h"
#include "../Framework/Renderable.h"

class Material;

// #define BUFFER_COPY // 파티클 데이터에 관한 정보를 복사하려면 주석을 해제한다.
#define NUM_EMIT_THREAD 8.0f
#define NUM_UPDATE_THREAD 512.0f

/*
여러 이펙트를 구현하기 위해 다수의 파티클을
시뮬레이션하고 렌더링한다.
*/
class Particle : public Object, public Renderable
{
public:
	Particle(std::string&& name, int maxParticleNum);
	virtual ~Particle();

public:
	virtual void Tick(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

public:
	// 파티클에서 사용할 데이터 버퍼 및 카운터 버퍼를 생성한다.
	void CreateBuffers(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv);
	// 파티클 데이터를 업데이트한다.
	void Update(ID3D12GraphicsCommandList* cmdList);
	// 발생 시간이 되었다면 파티클을 생성한다.
	void Emit(ID3D12GraphicsCommandList* cmdList);
	// gpu에서부터 데이터를 cpu로 복사한다.
	void CopyData(ID3D12GraphicsCommandList* cmdList);
	// 버퍼를 SRV로서 파이프라인에 바인딩한다.
	void SetBufferSrv(ID3D12GraphicsCommandList* cmdList);
	// 버퍼를 UAV로서 파이프라인에 바인딩한다.
	void SetBufferUav(ID3D12GraphicsCommandList* cmdList);
	// 파티클의 상수 버퍼를 채울 데이터를 반환한다.
	void SetParticleConstants(ParticleConstants& constants);

	// 현재 파티클 상태를 확인하고 스폰 가능한지 여부를 반환한다.
	bool IsSpawnable() const;

	void SetMaterial(Material* material);
	Material* GetMaterial() const;

public:
	// 파티클의 데이터는 lifeTime에 따라
	// start에서 end로 변화한다.
	ParticleData start;
	ParticleData end;

	// 파티클에 관한 정보이다.
	float lifeTime = 0.0f;
	UINT32 emitNum = 1;

	// pair의 first는 최소 스폰 시간, second는 최대 스폰 시간이다.
	std::pair<float, float> spawnTimeRange;

	bool isActive = true;
	bool enabledGravity = false;
	bool isInfinite = false;

	UINT32 cbIndex = 0;

protected:
	INT32 maxParticleNum = 0;
	INT32 currentParticleNum = 0;
	bool isVisible = true;

private:
	Material* material = nullptr;

	float remainingSpawnTime = 0.0f;

	// 파티클 데이터들을 담는 버퍼
	Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer;
#ifdef BUFFER_COPY
	// 파티클 데이터들을 gpu에서 읽어들일 버퍼
	Microsoft::WRL::ComPtr<ID3D12Resource> readBackBuffer;
#endif

	// 현재 활성화되어 있는 파티클 개수를 셀 수 있는 카운터 버퍼
	Microsoft::WRL::ComPtr<ID3D12Resource> readBackCounter;
	// 0번쨰 값은 현재 존재하는 파티클의 수, 1번쨰 값은 할당할 인덱스의 값
	Microsoft::WRL::ComPtr<ID3D12Resource> bufferCounter;
};