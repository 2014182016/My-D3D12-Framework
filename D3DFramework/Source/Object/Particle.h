#pragma once

#include "Object.h"
#include "../Framework/Renderable.h"

class Material;

// #define BUFFER_COPY // ��ƼŬ �����Ϳ� ���� ������ �����Ϸ��� �ּ��� �����Ѵ�.
#define NUM_EMIT_THREAD 8.0f
#define NUM_UPDATE_THREAD 512.0f

/*
���� ����Ʈ�� �����ϱ� ���� �ټ��� ��ƼŬ��
�ùķ��̼��ϰ� �������Ѵ�.
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
	// ��ƼŬ���� ����� ������ ���� �� ī���� ���۸� �����Ѵ�.
	void CreateBuffers(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv);
	// ��ƼŬ �����͸� ������Ʈ�Ѵ�.
	void Update(ID3D12GraphicsCommandList* cmdList);
	// �߻� �ð��� �Ǿ��ٸ� ��ƼŬ�� �����Ѵ�.
	void Emit(ID3D12GraphicsCommandList* cmdList);
	// gpu�������� �����͸� cpu�� �����Ѵ�.
	void CopyData(ID3D12GraphicsCommandList* cmdList);
	// ���۸� SRV�μ� ���������ο� ���ε��Ѵ�.
	void SetBufferSrv(ID3D12GraphicsCommandList* cmdList);
	// ���۸� UAV�μ� ���������ο� ���ε��Ѵ�.
	void SetBufferUav(ID3D12GraphicsCommandList* cmdList);
	// ��ƼŬ�� ��� ���۸� ä�� �����͸� ��ȯ�Ѵ�.
	void SetParticleConstants(ParticleConstants& constants);

	// ���� ��ƼŬ ���¸� Ȯ���ϰ� ���� �������� ���θ� ��ȯ�Ѵ�.
	bool IsSpawnable() const;

	void SetMaterial(Material* material);
	Material* GetMaterial() const;

public:
	// ��ƼŬ�� �����ʹ� lifeTime�� ����
	// start���� end�� ��ȭ�Ѵ�.
	ParticleData start;
	ParticleData end;

	// ��ƼŬ�� ���� �����̴�.
	float lifeTime = 0.0f;
	UINT32 emitNum = 1;

	// pair�� first�� �ּ� ���� �ð�, second�� �ִ� ���� �ð��̴�.
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

	// ��ƼŬ �����͵��� ��� ����
	Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer;
#ifdef BUFFER_COPY
	// ��ƼŬ �����͵��� gpu���� �о���� ����
	Microsoft::WRL::ComPtr<ID3D12Resource> readBackBuffer;
#endif

	// ���� Ȱ��ȭ�Ǿ� �ִ� ��ƼŬ ������ �� �� �ִ� ī���� ����
	Microsoft::WRL::ComPtr<ID3D12Resource> readBackCounter;
	// 0���� ���� ���� �����ϴ� ��ƼŬ�� ��, 1���� ���� �Ҵ��� �ε����� ��
	Microsoft::WRL::ComPtr<ID3D12Resource> bufferCounter;
};