#pragma once

#include "Object.h."
#include "../Framework/Renderable.h"
#include <optional>
#include <any>

class Mesh;
class Material;

/*
다양한 메쉬를 그리거나 물리를 수행하는 클래스
*/
class GameObject : public Object, public Renderable
{
public:
	GameObject(std::string&& name);
	virtual ~GameObject();

public:
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

	// 매 프레임마다 객체가 변화했는 지 체크하고 월드 행렬을 계산한다. 
	virtual void CalculateWorld() override;

	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

public:
	// 현재 메쉬가 프러스텀에 포함되어 있는지 확인한다.
	bool IsInFrustum(BoundingFrustum* frustum) const;

	// 힘을 주어 가속도를 변화시킨다.
	void AddForce(const DirectX::XMFLOAT3& force);
	void AddForce(const float forceX, const float forceY, const float forceZ);
	void AddForceAtLocalPoint(const XMFLOAT3& force, const XMFLOAT3& point);
	void AddForceAtWorldPoint(const XMFLOAT3& force, const XMFLOAT3& point);

	// 충격을 주어 속도를 변화시킨다.
	void Impulse(const XMFLOAT3& impulse);
	void Impulse(const float impulseX, const float impulseY, const float impulseZ);

	void SetMass(const float mass);
	void SetCollisionEnabled(const bool value);

	// 역 관성 텐서를 적용한다. 관성 텐서를 계산하기 위해선
	// 질량 및 충돌 타입이 먼저 설정되어 있어야 한다.
	void SetInverseInertiaTensor();

	// 역 관성 텐서를 세계 공간으로 변환한다.
	void TransformInverseInertiaTensorToWorld();

	CollisionType GetMeshCollisionType() const;
	std::optional<XMMATRIX> GetBoundingWorld() const;

	CollisionType GetCollisionType() const;
	const std::any GetCollisionBounding() const;

	XMFLOAT3 GetVelocity() const;
	XMFLOAT3 GetAcceleration() const;

	float GetMass() const;
	float GetInvMass() const;

	void SetMesh(Mesh* mesh);
	Mesh* GetMesh() const;

	void SetMaterial(Material* material);
	Material* GetMaterial() const;

protected:
	// 물리 업데이트를 수행한다.
	void PhysicsUpdate(float deltaTime);

public:
	CollisionType collisionType;
	UINT32 cbIndex = 0;
	bool isVisible = true;
	bool isPhysics = false;

	// 물리 업데이트를 할 때마다 속도의 일부분을 줄여준다.
	float linearDamping = 0.9f;
	float angularDamping = 0.9f;

	// 다른 물체가 충돌 후 분리속도를 결정하는 반발 계수
	// [0, 1] 사이로 1에 가까울수록 잘 튕기고, 0에 가까울수록 튕기지 않는다.
	float restitution = 1.0f;

protected:
	std::any collisionBounding = nullptr;

	// 역관성 텐서를 사용하여 물체의 회전을 계산할 수 있다.
	XMFLOAT4X4 invInertiaTensor = Matrix4x4::Identity();
	
	// 물체의 속도이다.
	XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 angularVelocity = { 0.0f, 0.0f, 0.0f };

	// 물체의 가속도이다.
	XMFLOAT3 acceleration = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 angularAcceleration = { 0.0f, 0.0f, 0.0f };

	// 물체가 가해지는 힘의 누적이다. 달랑베르의 원리에 따라 
	// 힘을 누적하여 한번에 물리 계산을 수행할 수 있다.
	XMFLOAT3 forceAccum = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 torqueAccum = { 0.0f, 0.0f, 0.0f };

	// 물리 업데이트에선 질량의 역수를 취한 값이 사용된다.
	// 또한, 무한대의 질량을 나타낼 수 있다.
	float invMass = 0.0f;
	float mass = 0.0f;

private:
	Mesh* mesh = nullptr;
	Material* material = nullptr;
};