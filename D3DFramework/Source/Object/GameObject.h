#pragma once

#include "Object.h"

#define DEATH_Z -1000.0f
#define GA -9.8f

class GameObject : public Object
{
public:
	GameObject(std::string&& name);
	virtual ~GameObject();
	
public:
	virtual void WorldUpdate() override;
	virtual void Tick(float deltaTime) override;
	virtual void Collide(std::shared_ptr<GameObject> other);

public:
	bool IsInFrustum(const DirectX::BoundingFrustum& camFrustum) const;
	bool IsCollision(const std::shared_ptr<GameObject> other) const;

	void AddImpulse(DirectX::XMVECTOR impulse);
	void AddImpulse(DirectX::XMFLOAT3 impulse);
	void AddImpulse(float impulseX, float impulseY, float impulseZ);

public:
	inline class Material* GetMaterial() const { return mMaterial; }
	inline void SetMaterial(class Material* mat) { mMaterial = mat; }
	inline class MeshGeometry* GetMesh() const { return mMesh; }
	inline void SetMesh(class MeshGeometry* mesh) { mMesh = mesh; SetCollisionEnabled(true); }

	inline RenderLayer GetRenderLayer() const { return mRenderLayer; }
	inline void SetRenderLayer(RenderLayer layer) { mRenderLayer = layer; }

	inline void SetVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

	inline void SetPhysics(bool value) { mIsPhysics = value; }
	inline bool GetIsPhysics() const { return mIsPhysics; }

	inline void SetMass(float mass) { mMass = mass; mInvMass = 1.0f / mass; }
	inline float GetMass() const { return mMass; }
	inline float GetInvMass() const { return mInvMass; }

	inline void SetCof(float cof) { mCof = cof;}
	inline float GetCof() const { return mCof; }

	inline void SetVelocity(DirectX::XMFLOAT3 velocity) { mVelocity = velocity; }
	inline DirectX::XMFLOAT3 GetVelocity() const { return mVelocity; }

	inline CollisionType GetCollisionType() const { return mCollisionType; }
	void SetCollisionEnabled(bool value);

	inline const std::any& GetCollisionBounding() const { return mCollisionBounding; }
	std::optional<DirectX::XMMATRIX> GetBoundingWorld() const;

	inline void SetObjectIndex(UINT index) { mObjectIndex = index; }
	inline UINT GetObjectIndex() const { return mObjectIndex; }

protected:
	class Material* mMaterial = nullptr;
	class MeshGeometry* mMesh = nullptr;

	std::any mCollisionBounding = nullptr;
	CollisionType mCollisionType = CollisionType::None;

	DirectX::XMFLOAT3 mVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mAcceleration = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	float mMass = 0.0f;
	float mInvMass = 0.0f;
	float mCof = 0.6f; 	// Coefficient of Restitution(반발 계수)

private:
	RenderLayer mRenderLayer = RenderLayer::Opaque;

	UINT mObjectIndex = 0;
	bool mIsVisible = true;
	bool mIsPhysics = false;
};