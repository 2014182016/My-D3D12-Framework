#include "../PrecompiledHeader/pch.h"
#include "GameObject.h"
#include "../Component/Mesh.h"
#include "../Framework/Physics.h"
#include "../Framework/D3DInfo.h"

GameObject::GameObject(std::string&& name) : Object(std::move(name)) { }

GameObject::~GameObject() { }

void GameObject::BeginPlay()
{
	CalculateWorld();
}

void GameObject::CalculateWorld()
{
	__super::CalculateWorld();

	switch (GetMeshCollisionType())
	{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(mesh->GetCollisionBounding());
			BoundingBox outAABB;
			aabb.Transform(outAABB, GetWorld());
			collisionBounding = std::move(outAABB);
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mesh->GetCollisionBounding());
			BoundingOrientedBox outOBB;
			obb.Transform(outOBB, GetWorld());
			collisionBounding = std::move(outOBB);
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mesh->GetCollisionBounding());
			BoundingSphere outSphere;
			sphere.Transform(outSphere, GetWorld());
			collisionBounding = std::move(outSphere);
			break;
		}
	}
}

void GameObject::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	if (isPhysics)
	{
		AddForce(Vector3::Multiply(Physics::gravity, mass));

		// 물리법칙에 따라 위치 및 속도를 업데이트한다.
		PhysicsUpdate(deltaTime);
		SetPosition(position);
	}
}

void GameObject::PhysicsUpdate(float deltaTime)
{
	if (invMass <= FLT_EPSILON)
		return;

	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR rot = XMLoadFloat3(&rotation);

	XMVECTOR vel = XMLoadFloat3(&velocity);
	XMVECTOR angVel = XMLoadFloat3(&angularVelocity);

	XMVECTOR acc = XMLoadFloat3(&acceleration);
	XMVECTOR angAcc = XMLoadFloat3(&angularAcceleration);

	XMVECTOR force = XMLoadFloat3(&forceAccum);
	XMVECTOR torque = XMLoadFloat3(&torqueAccum);
	
	// 가속도에 힘을 적용한다.
	acc += force * invMass;
	// 토크에 의해 각가속도를 계산한다.
	angVel += XMVector3Transform(torque, XMLoadFloat4x4(&invInertiaTensor));

	// 속도를 업데이트한다.
	vel += acc * deltaTime;
	// 각속도를 업데이트한다.
	angVel += angAcc * deltaTime;

	// 드래그(마찰저항)을 적용한다.
	vel *= pow(linearDamping, deltaTime);
	angVel *= pow(angularDamping, deltaTime);

	// 위치를 업데이트한다.
	pos += vel * deltaTime;
	rot += angVel * deltaTime;

	position = Vector3::XMVectorToFloat3(pos);
	rotation = Vector3::XMVectorToFloat3(rot);

	velocity = Vector3::XMVectorToFloat3(vel);
	angularVelocity = Vector3::XMVectorToFloat3(angVel);

	// 한 프레임동안 누적한 힘을 초기화한다.
	forceAccum = { 0.0f, 0.0f, 0.0f };
	torqueAccum = { 0.0f, 0.0f, 0.0f };
}

std::optional<XMMATRIX> GameObject::GetBoundingWorld() const
{
	if (!collisionBounding.has_value())
		return {};

	switch (GetMeshCollisionType())
	{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(collisionBounding);
			XMMATRIX translation = XMMatrixTranslation(aabb.Center.x, aabb.Center.y, aabb.Center.z);
			XMMATRIX scailing = XMMatrixScaling(aabb.Extents.x, aabb.Extents.y, aabb.Extents.z);
			XMMATRIX world = XMMatrixMultiply(scailing, translation);
			return world;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(collisionBounding);
			XMMATRIX translation = XMMatrixTranslation(obb.Center.x, obb.Center.y, obb.Center.z);
			XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.Orientation));
			XMMATRIX scailing = XMMatrixScaling(obb.Extents.x, obb.Extents.y, obb.Extents.z);
			XMMATRIX world = XMMatrixMultiply(scailing, XMMatrixMultiply(rotation, translation));
			return world;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(collisionBounding);
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
			const BoundingBox& aabb = std::any_cast<BoundingBox>(collisionBounding);
			if ((*frustum).Contains(aabb) != DirectX::DISJOINT)
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(collisionBounding);
			if ((*frustum).Contains(obb) != DirectX::DISJOINT)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(collisionBounding);
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


void GameObject::SetMass(const float mass)
{
	if (mass >= FLT_MAX - 1.0f || mass <= 0.0f)
	{
		this->mass = 0.0f;
		return;
	}

	this->mass = mass;
	invMass = 1.0f / mass;
}

void GameObject::SetCollisionEnabled(const bool value)
{
	if (value)
	{
		collisionType = GetMeshCollisionType();
	}
	else
	{
		collisionType = CollisionType::None;
	}
}

CollisionType GameObject::GetMeshCollisionType() const
{
	if (mesh)
		return mesh->GetCollisionType();
	return CollisionType::None;
}

void GameObject::Render(ID3D12GraphicsCommandList* commandList, BoundingFrustum* frustum) const
{
	if (!isVisible || !mesh)
		return;

	if (!IsInFrustum(frustum))
		return;

	mesh->Render(commandList);
}

void GameObject::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress + cbIndex * ConstantsSize::objectCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView((UINT)RpCommon::Object, cbAddress);
}

void GameObject::AddForce(const XMFLOAT3& force)
{
	AddForce(force.x, force.y, force.z);
}

void GameObject::AddForce(const float forceX, const float forceY, const float forceZ)
{
	if (invMass <= FLT_EPSILON)
		return;

	forceAccum.x += forceX * invMass;
	forceAccum.y += forceY * invMass;
	forceAccum.z += forceZ * invMass;
}

void GameObject::Impulse(const XMFLOAT3& impulse)
{
	Impulse(impulse.x, impulse.y, impulse.z);
}

void GameObject::Impulse(const float impulseX, const float impulseY, const float impulseZ)
{
	if (invMass <= FLT_EPSILON)
		return;

	velocity.x += impulseX * invMass;
	velocity.y += impulseY * invMass;
	velocity.z += impulseZ * invMass;
}

void GameObject::SetInverseInertiaTensor()
{
	XMFLOAT4X4 inertiaTensor = Matrix4x4::Identity();

	switch (collisionType)
	{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(collisionBounding);
			XMFLOAT3 extents = aabb.Extents;

			inertiaTensor._11 = (mass * (extents.y * extents.y + extents.z * extents.z)) / 12.0f;
			inertiaTensor._22 = (mass * (extents.x * extents.x + extents.z * extents.z)) / 12.0f;
			inertiaTensor._33 = (mass * (extents.x * extents.x + extents.y * extents.y)) / 12.0f;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(collisionBounding);
			XMFLOAT3 extents = obb.Extents;

			inertiaTensor._11 = (mass * (extents.y * extents.y + extents.z * extents.z)) / 12.0f;
			inertiaTensor._22 = (mass * (extents.x * extents.x + extents.z * extents.z)) / 12.0f;
			inertiaTensor._33 = (mass * (extents.x * extents.x + extents.y * extents.y)) / 12.0f;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(collisionBounding);
			float radiusSquare = sphere.Radius * sphere.Radius;
			float tensor = (2.0f * mass * radiusSquare) / 5.0f;

			inertiaTensor._11 = inertiaTensor._22 = inertiaTensor._33 = tensor;
			break;
		}
	}

	invInertiaTensor = Matrix4x4::Inverse(inertiaTensor);
}

void GameObject::TransformInverseInertiaTensorToWorld()
{
	invInertiaTensor = Matrix4x4::Multiply(invInertiaTensor, world);
}

void GameObject::AddForceAtLocalPoint(const XMFLOAT3& force, const XMFLOAT3& point)
{
	XMFLOAT3 pt = Vector3::TransformNormal(point, XMLoadFloat4x4(&world));

	AddForceAtWorldPoint(force, pt);
}

void GameObject::AddForceAtWorldPoint(const XMFLOAT3& force, const XMFLOAT3& point)
{
	XMFLOAT3 pt = Vector3::Subtract(point, position);
	XMFLOAT3 addForce = Vector3::CrossProduct(pt, force);

	forceAccum = Vector3::Add(forceAccum, addForce);
	torqueAccum = Vector3::Add(torqueAccum, addForce);
}

CollisionType GameObject::GetCollisionType() const
{
	return collisionType;
}

const std::any GameObject::GetCollisionBounding() const
{
	return collisionBounding;
}

XMFLOAT3 GameObject::GetVelocity() const
{
	return velocity;
}

XMFLOAT3 GameObject::GetAcceleration() const
{
	return acceleration;
}

float GameObject::GetMass() const
{
	return mass;
}

float GameObject::GetInvMass() const
{
	return invMass;
}

void GameObject::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
}

Mesh* GameObject::GetMesh() const
{
	return mesh;
}

void GameObject::SetMaterial(Material* material)
{
	this->material = material;
}

Material* GameObject::GetMaterial() const
{
	return material;
}