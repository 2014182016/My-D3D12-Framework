#pragma once

#include "Object.h"

#define DEATH_Z -1000.0f
#define GA 9.8f

class GameObject : public Object
{
public:
	GameObject(std::string name);
	virtual ~GameObject();
	
public:
	virtual void WorldUpdate() override;
	virtual void Tick(float deltaTime) override;
	virtual void Collision(std::shared_ptr<GameObject> other);

public:
	bool IsInFrustum(const DirectX::BoundingFrustum& camFrustum) const;
	std::shared_ptr<GameObject> IsCollision(const std::shared_ptr<GameObject> other) const;

	void AddForce(DirectX::XMFLOAT3 force);
	void AddForce(float forceX, float forceY, float forceZ);

public:
	inline class Material* GetMaterial() const { return mMaterial; }
	inline void SetMaterial(class Material* mat) { mMaterial = mat; }
	inline class MeshGeometry* GetMesh() const { return mMesh; }
	inline void SetMesh(class MeshGeometry* mesh) { mMesh = mesh; SetUsingCollision(); }

	inline RenderLayer GetRenderLayer() const { return mRenderLayer; }
	inline void SetRenderLayer(RenderLayer layer) { mRenderLayer = layer; }

	inline void SetVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

	inline void SetPhysics(bool value) { mIsPhysics = value; mIsMovable = true; }
	inline bool GetIsPhysics() const { return mIsPhysics; }

	inline void SetMass(float mass) { mMass = mass; }
	inline float GetMass() const { return mMass; }

	inline void SetVelocity(DirectX::XMFLOAT3 velocity) { mVelocity = velocity; }
	inline DirectX::XMFLOAT3 GetVelocity() const { return mVelocity; }

	inline CollisionType GetCollisionType() const { return mCollisionType; }
	inline void SetCollisionNone() { mCollisionType = CollisionType::None; }
	void SetUsingCollision();

	inline const std::any& GetCollisionBounding() const { return mCollisionBounding; }
	std::optional<DirectX::XMMATRIX> GetBoundingWorld() const;

protected:
	class Material* mMaterial = nullptr;
	class MeshGeometry* mMesh = nullptr;

	std::any mCollisionBounding = nullptr;
	CollisionType mCollisionType = CollisionType::None;

	DirectX::XMFLOAT3 mVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mAcceleration = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	float mMass = 0.0f;

private:
	RenderLayer mRenderLayer = RenderLayer::Opaque;

	float cof = 0.6f;

	bool mIsVisible = true;
	bool mIsPhysics = false;
};