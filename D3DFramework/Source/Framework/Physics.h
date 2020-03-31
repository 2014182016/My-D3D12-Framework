#pragma once

#include "Vector.h"
#include <DirectXCollision.h>

class GameObject;

/*
Ray Castring 시 광선의 정보와
부딪친 객체의 정보를 넘겨주는 구조체
*/
struct HitInfo
{
	XMFLOAT3 rayOrigin = { 0.0f, 0.0f, 0.0f };
	float dist;
	XMFLOAT3 rayDirection = { 0.0f, 0.0f, 0.0f };
	void* obj;
};

/*
물체의 충돌 시 물리적인 상호작용을 수행하기 위해
필요한 물리 정보들
*/
struct ContactInfo
{
	XMFLOAT3 contactNormal = { 0.0f, 0.0f, 0.0f };
	float penetration = 0.0f;
	XMFLOAT3 contactPoint = { 0.0f, 0.0f, 0.0f };

	// 충돌 노멀은 보통 obj1->GetPosition() - obj2->GetPosition()으로
	// 정하지만 obj1과 obj2가 서로 뒤바뀐 경우 노멀방향도 바꾸어 주어야 한다.
	float normalDirection = 1.0f;
};

/*
물체들의 물리 상호작용을 구현한 클래스
*/
class Physics
{
public:
	static inline XMFLOAT3 gravity = { 0.0f, -9.8f, 0.0f };

public:
	// 물체와 물체 사이의 충돌을 확인한다.
	static bool IsCollision(GameObject* obj1, GameObject* obj2);
	// 물체와 광선 사이의 충돌을 확인한다.
	static bool IsCollision(GameObject* obj, const XMVECTOR& rayOrigin, const XMVECTOR& rayDir,
		float& dist, bool isMeshCollision = false);

	// 물체 사이의 충돌 시 필요한 물리 정보들을 반환한다.
	static struct ContactInfo Contact(GameObject* obj1, GameObject* obj2);
	// 물체가 충돌하였을 때, 물리 상호작용을 수행한다.
	static void Collide(GameObject* obj1, GameObject* obj2, const float deltaTime);

	// 물체가 서로 충돌하였을 때, 물체의 속력을 수정하여 튕기도록 한다.
	static void ResolveVelocity(GameObject* obj1, GameObject* obj2, const float deltaTime, const ContactInfo& contactInfo);
	// 물체가 서로 관통되었을 때, 물체의 위치를 수정하여 관통을 해결한다.
	static void ResolveInterpenetration(GameObject* obj1, GameObject* obj2, const float deltaTime, const ContactInfo& contactInfo);

	// 각 바운딩끼리의 충돌을 확인하고 물리 정보를 반환한다.
	static ContactInfo ContactSphereAndSphere(GameObject* obj1, GameObject* obj2);
	static ContactInfo ContactBoxAndSphere(GameObject* obj1, GameObject* obj2);
	static ContactInfo ContactAabbAndAabb(GameObject* obj1, GameObject* obj2);
	static ContactInfo ContactBoxAndBox(GameObject* obj1, GameObject* obj2);

	// 객체의 바운딩이 aabb내에 완전히 포함되어 있을지를 확인한다.
	static bool Contain(GameObject* obj, const BoundingBox& aabb);

};