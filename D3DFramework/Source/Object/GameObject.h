#pragma once

#include "Object.h."
#include "../Framework/Renderable.h"
#include <optional>
#include <any>

class Mesh;
class Material;

/*
�پ��� �޽��� �׸��ų� ������ �����ϴ� Ŭ����
*/
class GameObject : public Object, public Renderable
{
public:
	GameObject(std::string&& name);
	virtual ~GameObject();

public:
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

	// �� �����Ӹ��� ��ü�� ��ȭ�ߴ� �� üũ�ϰ� ���� ����� ����Ѵ�. 
	virtual void CalculateWorld() override;

	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

public:
	// ���� �޽��� �������ҿ� ���ԵǾ� �ִ��� Ȯ���Ѵ�.
	bool IsInFrustum(BoundingFrustum* frustum) const;

	// ���� �־� ���ӵ��� ��ȭ��Ų��.
	void AddForce(const DirectX::XMFLOAT3& force);
	void AddForce(const float forceX, const float forceY, const float forceZ);
	void AddForceAtLocalPoint(const XMFLOAT3& force, const XMFLOAT3& point);
	void AddForceAtWorldPoint(const XMFLOAT3& force, const XMFLOAT3& point);

	// ����� �־� �ӵ��� ��ȭ��Ų��.
	void Impulse(const XMFLOAT3& impulse);
	void Impulse(const float impulseX, const float impulseY, const float impulseZ);

	void SetMass(const float mass);
	void SetCollisionEnabled(const bool value);

	// �� ���� �ټ��� �����Ѵ�. ���� �ټ��� ����ϱ� ���ؼ�
	// ���� �� �浹 Ÿ���� ���� �����Ǿ� �־�� �Ѵ�.
	void SetInverseInertiaTensor();

	// �� ���� �ټ��� ���� �������� ��ȯ�Ѵ�.
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
	// ���� ������Ʈ�� �����Ѵ�.
	void PhysicsUpdate(float deltaTime);

public:
	CollisionType collisionType;
	UINT32 cbIndex = 0;
	bool isVisible = true;
	bool isPhysics = false;

	// ���� ������Ʈ�� �� ������ �ӵ��� �Ϻκ��� �ٿ��ش�.
	float linearDamping = 0.9f;
	float angularDamping = 0.9f;

	// �ٸ� ��ü�� �浹 �� �и��ӵ��� �����ϴ� �ݹ� ���
	// [0, 1] ���̷� 1�� �������� �� ƨ���, 0�� �������� ƨ���� �ʴ´�.
	float restitution = 1.0f;

protected:
	std::any collisionBounding = nullptr;

	// ������ �ټ��� ����Ͽ� ��ü�� ȸ���� ����� �� �ִ�.
	XMFLOAT4X4 invInertiaTensor = Matrix4x4::Identity();
	
	// ��ü�� �ӵ��̴�.
	XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 angularVelocity = { 0.0f, 0.0f, 0.0f };

	// ��ü�� ���ӵ��̴�.
	XMFLOAT3 acceleration = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 angularAcceleration = { 0.0f, 0.0f, 0.0f };

	// ��ü�� �������� ���� �����̴�. �޶������� ������ ���� 
	// ���� �����Ͽ� �ѹ��� ���� ����� ������ �� �ִ�.
	XMFLOAT3 forceAccum = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 torqueAccum = { 0.0f, 0.0f, 0.0f };

	// ���� ������Ʈ���� ������ ������ ���� ���� ���ȴ�.
	// ����, ���Ѵ��� ������ ��Ÿ�� �� �ִ�.
	float invMass = 0.0f;
	float mass = 0.0f;

private:
	Mesh* mesh = nullptr;
	Material* material = nullptr;
};