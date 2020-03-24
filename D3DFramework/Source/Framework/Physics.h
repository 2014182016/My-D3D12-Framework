#pragma once

#include "pch.h"

class Physics
{
public:
	static inline DirectX::XMFLOAT3 gravity = DirectX::XMFLOAT3(0.0f, -9.8f, 0.0f);

public:
	static bool IsCollision(class GameObject* obj1, class GameObject* obj2);
	static bool IsCollision(class GameObject* obj, const DirectX::XMVECTOR& rayOrigin, const DirectX::XMVECTOR& rayDir,
		float& dist, bool isMeshCollision = false);

	static struct ContactInfo Contact(class GameObject* obj1, class GameObject* obj2);
	static void Collide(class GameObject* obj1, class GameObject* obj2, const float deltaTime);

	static void ResolveVelocity(class GameObject* obj1, class GameObject* obj2, const float deltaTime, 
		const struct ContactInfo& contactInfo);
	static void ResolveInterpenetration(class GameObject* obj1, class GameObject* obj2, const float deltaTime, 
		const struct ContactInfo& contactInfo);

	static struct ContactInfo ContactSphereAndSphere(class GameObject* obj1, class GameObject* obj2);
	static struct ContactInfo ContactBoxAndSphere(class GameObject* obj1, class GameObject* obj2);
	static struct ContactInfo ContactAabbAndAabb(class GameObject* obj1, class GameObject* obj2);
	static struct ContactInfo ContactBoxAndBox(class GameObject* obj1, class GameObject* obj2);

	static struct ContactInfo Contact(class GameObject* obj1, const DirectX::BoundingSphere& sphere);

	static bool Contain(class GameObject* obj, const DirectX::BoundingBox& aabb);

};