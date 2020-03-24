#include "pch.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Global.h"
#include "Structure.h"
#include "Physics.h"

using namespace DirectX;

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
			const BoundingBox& aabb = std::any_cast<BoundingBox>(mMesh->GetCollisionBounding());
			BoundingBox outAABB;

			aabb.Transform(outAABB, GetWorld());
			mCollisionBounding = std::move(outAABB);
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mMesh->GetCollisionBounding());
			BoundingOrientedBox outOBB;
			obb.Transform(outOBB, GetWorld());
			mCollisionBounding = std::move(outOBB);
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mMesh->GetCollisionBounding());
			BoundingSphere outSphere;
			sphere.Transform(outSphere, GetWorld());
			mCollisionBounding = std::move(outSphere);
			break;
		}
	}
}

void GameObject::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	if (mIsPhysics)
	{
		AddForce(Physics::gravity * mMass);

		// 물리법칙에 따라 위치 및 속도를 업데이트한다.
		PhysicsUpdate(deltaTime);
		SetPosition(mPosition);
	}
}

void GameObject::PhysicsUpdate(float deltaTime)
{
	if (mInvMass <= FLT_EPSILON)
		return;

	XMVECTOR position = XMLoadFloat3(&mPosition);
	XMVECTOR rotation = XMLoadFloat3(&mRotation);

	XMVECTOR velocity = XMLoadFloat3(&mVelocity);
	XMVECTOR angularVelocity = XMLoadFloat3(&mAngularVelocity);

	XMVECTOR acceleration = XMLoadFloat3(&mAcceleration);
	XMVECTOR angularAcceleration = XMLoadFloat3(&mAngularAcceleration);

	XMVECTOR forceAccum = XMLoadFloat3(&mForceAccum);
	XMVECTOR torqueAccum = XMLoadFloat3(&mTorqueAccum);
	
	// 가속도에 힘을 적용한다.
	acceleration += forceAccum * mInvMass;
	// 토크에 의해 각가속도를 계산한다.
	angularAcceleration += XMVector3Transform(torqueAccum, XMLoadFloat4x4(&mInvInertiaTensor));

	// 속도를 업데이트한다.
	velocity += acceleration * deltaTime;
	// 각속도를 업데이트한다.
	angularVelocity += angularAcceleration * deltaTime;

	// 드래그(마찰저항)을 적용한다.
	velocity *= pow(mLinearDamping, deltaTime);
	angularVelocity *= pow(mAngularDamping, deltaTime);

	// 위치를 업데이트한다.
	position += velocity * deltaTime;
	rotation += angularVelocity * deltaTime;

	XMStoreFloat3(&mPosition, position);
	XMStoreFloat3(&mRotation, rotation);

	XMStoreFloat3(&mVelocity, velocity);
	XMStoreFloat3(&mAngularVelocity, angularVelocity);

	// 한 프레임동안 누적한 힘을 초기화한다.
	mForceAccum = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mTorqueAccum = XMFLOAT3(0.0f, 0.0f, 0.0f);
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


void GameObject::SetMass(float mass)
{
	if (mass >= FLT_MAX - 1.0f || mass <= 0.0f)
	{
		mass = 0.0f;
		return;
	}

	mMass = mass;
	mInvMass = 1.0f / mass;
}

void GameObject::SetCollisionEnabled(bool value)
{
	if (value)
	{
		mCollisionType = GetMeshCollisionType();
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

void GameObject::AddForce(const XMFLOAT3& force)
{
	AddForce(force.x, force.y, force.z);
}

void GameObject::AddForce(float forceX, float forceY, float forceZ)
{
	if (mInvMass <= FLT_EPSILON)
		return;

	mForceAccum.x += forceX * mInvMass;
	mForceAccum.y += forceY * mInvMass;
	mForceAccum.z += forceZ * mInvMass;
}

void GameObject::Impulse(const XMFLOAT3& impulse)
{
	Impulse(impulse.x, impulse.y, impulse.z);
}

void GameObject::Impulse(float impulseX, float impulseY, float impulseZ)
{
	if (mInvMass <= FLT_EPSILON)
		return;

	mVelocity.x += impulseX * mInvMass;
	mVelocity.y += impulseY * mInvMass;
	mVelocity.z += impulseZ * mInvMass;
}

void GameObject::SetInverseInertiaTensor()
{
	XMFLOAT4X4 inertiaTensor = Identity4x4f();

	switch (mCollisionType)
	{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(mCollisionBounding);
			XMFLOAT3 extents = aabb.Extents;

			inertiaTensor._11 = (mMass * (extents.y * extents.y + extents.z * extents.z)) / 12.0f;
			inertiaTensor._22 = (mMass * (extents.x * extents.x + extents.z * extents.z)) / 12.0f;
			inertiaTensor._33 = (mMass * (extents.x * extents.x + extents.y * extents.y)) / 12.0f;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(mCollisionBounding);
			XMFLOAT3 extents = obb.Extents;

			inertiaTensor._11 = (mMass * (extents.y * extents.y + extents.z * extents.z)) / 12.0f;
			inertiaTensor._22 = (mMass * (extents.x * extents.x + extents.z * extents.z)) / 12.0f;
			inertiaTensor._33 = (mMass * (extents.x * extents.x + extents.y * extents.y)) / 12.0f;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(mCollisionBounding);
			float radiusSquare = sphere.Radius * sphere.Radius;
			float tensor = (2.0f * mMass * radiusSquare) / 5.0f;

			inertiaTensor._11 = inertiaTensor._22 = inertiaTensor._33 = tensor;
			break;
		}
	}

	XMMATRIX matInertiaTensor = XMLoadFloat4x4(&inertiaTensor);
	XMMATRIX matInvInertiaTensor = XMMatrixInverse(&XMMatrixDeterminant(matInertiaTensor), matInertiaTensor);

	XMStoreFloat4x4(&mInvInertiaTensor, matInvInertiaTensor);
}

void GameObject::TransformInverseInertiaTensorToWorld()
{
	XMMATRIX invInertiaTensor = XMLoadFloat4x4(&mInvInertiaTensor);
	XMMATRIX world = XMLoadFloat4x4(&mWorld);

	invInertiaTensor = invInertiaTensor * world;
	XMStoreFloat4x4(&mInvInertiaTensor, invInertiaTensor);
}

void GameObject::AddForceAtLocalPoint(const XMFLOAT3& force, const XMFLOAT3& point)
{
	XMVECTOR worldPoint = XMVector3Transform(XMLoadFloat3(&point), XMLoadFloat4x4(&mWorld));
	XMFLOAT3 pt;
	XMStoreFloat3(&pt, worldPoint);

	AddForceAtWorldPoint(force, pt);
}

void GameObject::AddForceAtWorldPoint(const XMFLOAT3& force, const XMFLOAT3& point)
{
	XMVECTOR position = XMLoadFloat3(&mPosition);
	XMVECTOR pt = XMLoadFloat3(&point);
	XMVECTOR fr = XMLoadFloat3(&force);

	pt = pt - position;
	fr = XMVector3Cross(pt, fr);

	mForceAccum = mForceAccum + force;
	XMStoreFloat3(&mTorqueAccum, fr);
}