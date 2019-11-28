#include "pch.h"
#include "GameObject.h"
#include "MeshGeometry.h"
#include "D3DFramework.h"

using namespace DirectX;

GameObject::GameObject(std::string name) : Object(name) { }

GameObject::~GameObject() { }


void GameObject::WorldUpdate()
{
	__super::WorldUpdate();

	if (mMesh)
	{
		if (mMesh->GetCollisionBounding().has_value())
		{
			switch (mMesh->GetCollisionType())
			{
			case CollisionType::AABB:
			{
				const BoundingBox& aabb = std::any_cast<BoundingBox>(mMesh->GetCollisionBounding());
				BoundingBox out;
				aabb.Transform(out, XMLoadFloat4x4(&mWorld));
				mCollisionBounding = std::move(out);
				break;
			}
			case CollisionType::OBB:
			{
				const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mMesh->GetCollisionBounding());
				BoundingOrientedBox out;
				obb.Transform(out, XMLoadFloat4x4(&mWorld));
				mCollisionBounding = std::move(out);
				break;
			}
			case CollisionType::Sphere:
			{
				const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mMesh->GetCollisionBounding());
				BoundingSphere out;
				sphere.Transform(out, XMLoadFloat4x4(&mWorld));
				mCollisionBounding = std::move(out);
				break;
			}
			}
		}
	}
}

void GameObject::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	if (mIsPhysics)
	{
		if (mMass < FLT_EPSILON)
		{
#if defined(DEBUG) || defined(_DEBUG)
			std::cout << ToString() << " : Mass 0" << std::endl;
#endif
			return;
		}

		// 중력 적용
		mVelocity.y += -GA * deltaTime;

		// 가속도 적용
		mVelocity.x += mAcceleration.x * deltaTime;
		mVelocity.y += mAcceleration.y * deltaTime;
		mVelocity.z += mAcceleration.z * deltaTime;
		
		// 위치 적용
		Move(mVelocity.x * deltaTime, mVelocity.y * deltaTime, mVelocity.z * deltaTime);

		if (mPosition.y < DEATH_Z)
			Destroy();
	}
}

void GameObject::Collision(std::shared_ptr<GameObject> other)
{

}

std::optional<XMMATRIX> GameObject::GetBoundingWorld() const
{
	if (!mCollisionBounding.has_value())
		return {};

	if (mMesh)
	{
		switch (mMesh->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(mCollisionBounding);
			XMMATRIX translation = XMMatrixTranslation(aabb.Center.x, aabb.Center.y, aabb.Center.z);
			XMMATRIX scailing = XMMatrixScaling(aabb.Extents.x, aabb.Extents.y, aabb.Extents.z);
			XMMATRIX world = XMMatrixMultiply(scailing, translation);
			return world;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mCollisionBounding);
			XMMATRIX translation = XMMatrixTranslation(obb.Center.x, obb.Center.y, obb.Center.z);
			XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.Orientation));
			XMMATRIX scailing = XMMatrixScaling(obb.Extents.x, obb.Extents.y, obb.Extents.z);
			XMMATRIX world = XMMatrixMultiply(scailing, XMMatrixMultiply(rotation, translation));
			return world;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mCollisionBounding);
			XMMATRIX translation = XMMatrixTranslation(sphere.Center.x, sphere.Center.y, sphere.Center.z);
			XMMATRIX scailing = XMMatrixScaling(sphere.Radius, sphere.Radius, sphere.Radius);
			XMMATRIX world = XMMatrixMultiply(scailing, translation);
			return world;
		}
		}
	}

	return {};
}

bool GameObject::IsInFrustum(const DirectX::BoundingFrustum& camFrustum) const
{
	if (mMesh)
	{
		switch (mMesh->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(mCollisionBounding);
			if (camFrustum.Contains(aabb) != DirectX::DISJOINT)
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mCollisionBounding);
			if (camFrustum.Contains(obb) != DirectX::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mCollisionBounding);
			if (camFrustum.Contains(sphere) != DirectX::DISJOINT)
				return true;
			break;
		}
		default:
			return true;
		}
	}

	return false;
}

void GameObject::SetUsingCollision() 
{
	if (mMesh) 
		mCollisionType = mMesh->GetCollisionType(); 
}

std::shared_ptr<GameObject> GameObject::IsCollision(const std::shared_ptr<GameObject> other) const
{
	if (mCollisionType == CollisionType::None || other->GetCollisionType() == CollisionType::None)
		return nullptr;

	switch (mCollisionType) 
	{
	case CollisionType::AABB:
	{
		const BoundingBox& aabb = std::any_cast<BoundingBox>(mCollisionBounding);
		switch (other->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& otherAabb = std::any_cast<BoundingBox>(other->GetCollisionBounding());
			if (aabb.Contains(otherAabb) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(other->GetCollisionBounding());
			if (aabb.Contains(otherObb) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(other->GetCollisionBounding());
			if (aabb.Contains(otherSphere) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		}
		break;
	}
	case CollisionType::OBB:
	{
		const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mCollisionBounding);
		switch (other->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& otherAabb = std::any_cast<BoundingBox>(other->GetCollisionBounding());
			if (obb.Contains(otherAabb) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(other->GetCollisionBounding());
			if (obb.Contains(otherObb) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(other->GetCollisionBounding());
			if (obb.Contains(otherSphere) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		}
		break;
	}
	case CollisionType::Sphere:
	{
		const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mCollisionBounding);
		switch (other->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& otherAabb = std::any_cast<BoundingBox>(other->GetCollisionBounding());
			if (sphere.Contains(otherAabb) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(other->GetCollisionBounding());
			if (sphere.Contains(otherObb) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(other->GetCollisionBounding());
			if (sphere.Contains(otherSphere) != ContainmentType::DISJOINT)
				return other;
			break;
		}
		}
		break;
	}
	}

	return nullptr;
}

void GameObject::AddForce(DirectX::XMFLOAT3 force)
{
	AddForce(force.x, force.y, force.z);
}

void GameObject::AddForce(float forceX, float forceY, float forceZ)
{
	if (mMass < FLT_EPSILON)
		return;

	mAcceleration.x = forceX / mMass;
	mAcceleration.y = forceY / mMass;
	mAcceleration.z = forceZ / mMass;
}