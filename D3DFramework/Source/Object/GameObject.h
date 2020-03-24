#pragma once

#include "Object.h"
#include "Interface.h"

class GameObject : public Object, public Renderable
{
public:
	GameObject(std::string&& name);
	virtual ~GameObject();

public:
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;
	virtual void CalculateWorld() override;

public:
	bool IsInFrustum(DirectX::BoundingFrustum* frustum) const;

	void AddForce(const DirectX::XMFLOAT3& force);
	void AddForce(float forceX, float forceY, float forceZ);
	void AddForceAtLocalPoint(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& point);
	void AddForceAtWorldPoint(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& point);
	void Impulse(const DirectX::XMFLOAT3& impulse);
	void Impulse(float impulseX, float impulseY, float impulseZ);

public:
	void SetMass(float mass);
	void SetCollisionEnabled(bool value);

	// �� ���� �ټ��� �����Ѵ�.
	// ���� �ټ��� ����ϱ� ���ؼ� ���� �� �浹 Ÿ����
	// ���� �����Ǿ� �־�� �Ѵ�.
	void SetInverseInertiaTensor();

	// �� ���� �ټ��� ���� �������� ��ȯ�Ѵ�.
	void TransformInverseInertiaTensorToWorld();

	CollisionType GetMeshCollisionType() const;
	std::optional<DirectX::XMMATRIX> GetBoundingWorld() const;

public:
	inline class Mesh* GetMesh() { return mMesh; }
	inline class Material* GetMaterial() { return mMaterial; }
	inline void SetMesh(class Mesh* mesh) { mMesh = mesh; }
	inline void SetMaterial(class Material* material) { mMaterial = material; }

	inline void SetVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }
	inline void SetPhysics(bool value) { mIsPhysics = value; }

	inline void SetCBIndex(UINT index) { mCBIndex = index; }
	inline UINT GetCBIndex() const { return mCBIndex; }

	inline CollisionType GetCollisionType() const { return mCollisionType; }
	inline const std::any& GetCollisionBounding() const { return mCollisionBounding; }

	inline void SetVelocity(const DirectX::XMFLOAT3& vel) { mVelocity = vel; }
	inline DirectX::XMFLOAT3 GetVelocity() const { return mVelocity; }

	inline void SetAcceleration(const DirectX::XMFLOAT3& accel) { mAcceleration = accel; }
	inline DirectX::XMFLOAT3 GetAcceleration() const { return mAcceleration; }

	inline float GetMass() const { return mMass; }
	inline float GetInvMass() const { return mInvMass; }

	inline void SetRestitution(float restitution) { mRestitution = restitution; }
	inline float GetRestitution() const { return mRestitution; }

	inline void SetLinearDamping(float damping) { mLinearDamping = damping; }
	inline void SetAngularDamping(float damping) { mAngularDamping = damping; }

protected:
	void PhysicsUpdate(float deltaTime);

protected:
	std::any mCollisionBounding = nullptr;
	CollisionType mCollisionType = CollisionType::None;

	class Mesh* mMesh = nullptr;
	class Material* mMaterial = nullptr;

	DirectX::XMFLOAT4X4 mInvInertiaTensor = DirectX::Identity4x4f();

	DirectX::XMFLOAT3 mVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mAngularVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	DirectX::XMFLOAT3 mAcceleration = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mAngularAcceleration = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	DirectX::XMFLOAT3 mForceAccum = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mTorqueAccum = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	// ���� ������Ʈ�� �� ������ �ӵ��� �Ϻκ��� �ٿ��ش�.
	float mLinearDamping = 0.9f;
	float mAngularDamping = 0.9f;

	// ���� ������Ʈ���� ������ ������ ���� ���� ���ȴ�.
	// ����, ���Ѵ��� ������ ��Ÿ�� �� �ִ�.
	float mInvMass = 0.0f;
	float mMass = 0.0f;

	// �ٸ� ��ü�� �浹 �� �и��ӵ��� �����ϴ� �ݹ� ���
	// [0, 1] ���̷� 1�� �������� �� ƨ���, 0�� �������� ƨ���� �ʴ´�.
	float mRestitution = 1.0f;

private:
	UINT mCBIndex = 0;
	bool mIsVisible = true;
	bool mIsPhysics = false;
};