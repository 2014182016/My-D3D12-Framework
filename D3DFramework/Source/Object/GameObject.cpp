#include "pch.h"
#include "GameObject.h"
#include "MeshGeometry.h"
#include "D3DFramework.h"

using namespace DirectX;

GameObject::GameObject(std::string&& name) : Object(std::move(name)) { }

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
		mVelocity.y += GA * deltaTime;

		// 가속도 적용
		mVelocity = mVelocity + mAcceleration * deltaTime;
		
		// 위치 적용
		Move(mVelocity * deltaTime);

		if (mPosition.y < DEATH_Z)
			Destroy();
	}
}

void GameObject::Collide(std::shared_ptr<GameObject> other)
{
	/*
	XMVECTOR myVel = XMLoadFloat3(&mVelocity);
	XMVECTOR otherVel = XMLoadFloat3(&other->GetVelocity());

	// 상대속도를 계산
	XMVECTOR relativeVel = otherVel - myVel;

	float velAlongNormal = XMVector3Dot(relativeVel, collisonNormal);
	if (velAlongNormal > 0)
		return;

	// 직관적인 결과를 위해 최저 반발을 이용한다.
	float e = std::min<float>(mCof, other->GetCof());

	// Impulse Scalar를 계산한다.
	float j = -(1 + e) * velAlongNormal;
	j /= mInvMass + other->GetInvMass();

	// 충격량만큼 힘을 준다.
	XMVECTOR impulse = j * collisonNormal;
	AddImpulse(-impulse);
	other->AddImpulse(impulse);
	*/
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

void GameObject::SetCollisionEnabled(bool value)
{
	if (value)
	{
		if (mMesh)
			mCollisionType = mMesh->GetCollisionType();
	}
	else
	{
		mCollisionType = CollisionType::None;
	}
}


bool GameObject::IsCollision(const std::shared_ptr<GameObject> other) const
{
	if (mCollisionType == CollisionType::None || other->GetCollisionType() == CollisionType::None)
		return false;

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
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(other->GetCollisionBounding());
			if (aabb.Contains(otherObb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(other->GetCollisionBounding());
			if (aabb.Contains(otherSphere) != ContainmentType::DISJOINT)
				return true;
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
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(other->GetCollisionBounding());
			if (obb.Contains(otherObb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(other->GetCollisionBounding());
			if (obb.Contains(otherSphere) != ContainmentType::DISJOINT)
				return true;
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
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(other->GetCollisionBounding());
			if (sphere.Contains(otherObb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(other->GetCollisionBounding());
			if (sphere.Contains(otherSphere) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		}
		break;
	}
	}

	return false;
}

void GameObject::AddImpulse(DirectX::XMVECTOR impulse)
{
	XMFLOAT3 xmf3Impulse;
	XMStoreFloat3(&xmf3Impulse, impulse);
	AddImpulse(xmf3Impulse);
}

void GameObject::AddImpulse(DirectX::XMFLOAT3 impulse)
{
	AddImpulse(impulse.x, impulse.y, impulse.z);
}

void GameObject::AddImpulse(float impulseX, float impulseY, float impulseZ)
{
	if (mMass < FLT_EPSILON || !mIsPhysics)
		return;

	mAcceleration.x = impulseX * mInvMass;
	mAcceleration.y = impulseY * mInvMass;
	mAcceleration.z = impulseZ * mInvMass;
}