#pragma once

#include "Object.h"
#include "../Framework/Interfaces.h"

#define DEATH_Z -1000.0f
#define GA -9.8f

class GameObject : public Object, public Renderable, public Physics
{
public:
	GameObject(std::string&& name);
	virtual ~GameObject();
	
public:
	virtual void WorldUpdate() override;
	virtual void Tick(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* commandList) override;
	virtual void Collide(std::shared_ptr<GameObject> other);

public:
	bool IsInFrustum(DirectX::BoundingFrustum* camFrustum) const;
	bool IsCollision(const std::shared_ptr<GameObject> other) const;

	void AddImpulse(DirectX::XMVECTOR impulse);
	void AddImpulse(DirectX::XMFLOAT3 impulse);
	void AddImpulse(float impulseX, float impulseY, float impulseZ);

public:
	inline CollisionType GetCollisionType() const { return mCollisionType; }
	void SetCollisionEnabled(bool value);

	inline const std::any& GetCollisionBounding() const { return mCollisionBounding; }
	std::optional<DirectX::XMMATRIX> GetBoundingWorld() const;

protected:
	std::any mCollisionBounding = nullptr;
	CollisionType mCollisionType = CollisionType::None;
};