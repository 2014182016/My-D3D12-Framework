#include "pch.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Global.h"

using namespace DirectX;

GameObject::GameObject(std::string&& name) : Object(std::move(name)) { }

GameObject::~GameObject() { }

void GameObject::BeginPlay()
{
	WorldUpdate();
}

void GameObject::WorldUpdate()
{
	__super::WorldUpdate();

	if (mMesh)
	{
		auto meshBounding = mMesh->GetCollisionBounding();
		if (meshBounding.has_value())
		{
			switch (mMesh->GetCollisionType())
			{
			case CollisionType::AABB:
			{
				const BoundingBox& aabb = std::any_cast<BoundingBox>(meshBounding);
				BoundingBox outAABB;
				aabb.Transform(outAABB, XMLoadFloat4x4(&mWorld));
				mCollisionBounding = std::move(outAABB);
				break;
			}
			case CollisionType::OBB:
			{
				const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(meshBounding);
				BoundingOrientedBox outOBB;
				obb.Transform(outOBB, XMLoadFloat4x4(&mWorld));
				mCollisionBounding = std::move(outOBB);
				break;
			}
			case CollisionType::Sphere:
			{
				const BoundingSphere& sphere = std::any_cast<BoundingSphere>(meshBounding);
				BoundingSphere outSphere;
				sphere.Transform(outSphere, XMLoadFloat4x4(&mWorld));
				mCollisionBounding = std::move(outSphere);
				break;
			}
			}
		}
	}
}

void GameObject::Collide(GameObject* other)
{

}

std::optional<XMMATRIX> GameObject::GetBoundingWorld() const
{
	if (!mCollisionBounding.has_value())
		return {};

	switch (GetMeshCollisionType())
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

	return {};
}

bool GameObject::IsInFrustum(DirectX::BoundingFrustum* frustum) const
{
	if (frustum == nullptr)
		return true;

	switch (GetMeshCollisionType())
	{
	case CollisionType::AABB:
	{
		const BoundingBox& aabb = std::any_cast<BoundingBox>(mCollisionBounding);
		if ((*frustum).Contains(aabb) != DirectX::DISJOINT)
			return true;
		break;
	}
	case CollisionType::OBB:
	{
		const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mCollisionBounding);
		if ((*frustum).Contains(obb) != DirectX::DISJOINT)
			return true;
		break;
	}
	case CollisionType::Sphere:
	{
		const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mCollisionBounding);
		if ((*frustum).Contains(sphere) != DirectX::DISJOINT)
			return true;
		break;
	}
	case CollisionType::Point:
	{
		XMFLOAT3 pos = GetPosition();
		XMVECTOR vecPos = XMLoadFloat3(&pos);
		if ((*frustum).Contains(vecPos) != DirectX::DISJOINT)
			return true;
		break;
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

CollisionType GameObject::GetMeshCollisionType() const
{
	if (mMesh)
		return mMesh->GetCollisionType();
	return CollisionType::None;
}

bool GameObject::IsCollision(GameObject* other) const
{
	CollisionType otherType = other->GetCollisionType();
	if (mCollisionType == CollisionType::None || otherType == CollisionType::None)
		return false;

	auto otherBounding = other->GetCollisionBounding();
	switch (mCollisionType)
	{
	case CollisionType::AABB:
	{
		const BoundingBox& aabb = std::any_cast<BoundingBox>(mCollisionBounding);
		switch (otherType)
		{
		case CollisionType::AABB:
		{
			const BoundingBox& otherAabb = std::any_cast<BoundingBox>(otherBounding);
			if (aabb.Contains(otherAabb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(otherBounding);
			if (aabb.Contains(otherObb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(otherBounding);
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
		switch (otherType)
		{
		case CollisionType::AABB:
		{
			const BoundingBox& otherAabb = std::any_cast<BoundingBox>(otherBounding);
			if (obb.Contains(otherAabb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(otherBounding);
			if (obb.Contains(otherObb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(otherBounding);
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
		switch (otherType)
		{
		case CollisionType::AABB:
		{
			const BoundingBox& otherAabb = std::any_cast<BoundingBox>(otherBounding);
			if (sphere.Contains(otherAabb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& otherObb = std::any_cast<BoundingOrientedBox>(otherBounding);
			if (sphere.Contains(otherObb) != ContainmentType::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& otherSphere = std::any_cast<BoundingSphere>(otherBounding);
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

void GameObject::Render(ID3D12GraphicsCommandList* commandList, BoundingFrustum* frustum) const
{
	if (!mIsVisible || !mMesh)
		return;

	if (!IsInFrustum(frustum))
		return;

	mMesh->Render(commandList);
}

void GameObject::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress + mCBIndex * ConstantsSize::objectCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView((UINT)RpCommon::Object, cbAddress);
}