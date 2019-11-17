#include "pch.h"
#include "GameObject.h"
#include "MeshGeometry.h"
#include "D3DFramework.h"

using namespace DirectX;

GameObject::GameObject(std::string name) : Object(name) { }

GameObject::~GameObject() { }

void GameObject::Destroy()
{
	std::forward_list<std::unique_ptr<class GameObject>>& gameObjects = D3DFramework::GetApp()->GetGameObjects((int)mRenderLayer);

	GameObject* objPtr = this;
	gameObjects.remove_if([&objPtr](const std::unique_ptr<GameObject>& obj)
	{ return obj.get() == objPtr; });
}

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
				mWorldBounding = std::move(out);
				break;
			}
			case CollisionType::OBB:
			{
				const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mMesh->GetCollisionBounding());
				BoundingOrientedBox out;
				obb.Transform(out, XMLoadFloat4x4(&mWorld));
				mWorldBounding = std::move(out);
				break;
			}
			case CollisionType::Sphere:
			{
				const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mMesh->GetCollisionBounding());
				BoundingSphere out;
				sphere.Transform(out, XMLoadFloat4x4(&mWorld));
				mWorldBounding = std::move(out);
				break;
			}
			}
		}
	}
}

std::optional<XMMATRIX> GameObject::GetBoundingWorld() const
{
	if (!mWorldBounding.has_value())
		return {};

	if (mMesh)
	{
		switch (mMesh->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(mWorldBounding);
			XMMATRIX translation = XMMatrixTranslation(aabb.Center.x, aabb.Center.y, aabb.Center.z);
			XMMATRIX scailing = XMMatrixScaling(aabb.Extents.x * 2.0f, aabb.Extents.y * 2.0f, aabb.Extents.z * 2.0f);
			XMMATRIX world = XMMatrixMultiply(scailing, translation);
			return world;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mWorldBounding);
			XMMATRIX translation = XMMatrixTranslation(obb.Center.x, obb.Center.y, obb.Center.z);
			XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.Orientation));
			XMMATRIX scailing = XMMatrixScaling(obb.Extents.x * 2.0f, obb.Extents.y * 2.0f, obb.Extents.z * 2.0f);
			XMMATRIX world = XMMatrixMultiply(scailing, XMMatrixMultiply(rotation, translation));
			return world;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mWorldBounding);
			XMMATRIX translation = XMMatrixTranslation(sphere.Center.x, sphere.Center.y, sphere.Center.z);
			XMMATRIX scailing = XMMatrixScaling(sphere.Radius, sphere.Radius, sphere.Radius);
			XMMATRIX world = XMMatrixMultiply(scailing, translation);
			return world;
		}
		}
	}

	return {};
}