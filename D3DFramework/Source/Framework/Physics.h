#pragma once

#include "Vector.h"
#include <DirectXCollision.h>

class GameObject;

/*
Ray Castring �� ������ ������
�ε�ģ ��ü�� ������ �Ѱ��ִ� ����ü
*/
struct HitInfo
{
	XMFLOAT3 rayOrigin = { 0.0f, 0.0f, 0.0f };
	float dist;
	XMFLOAT3 rayDirection = { 0.0f, 0.0f, 0.0f };
	void* obj;
};

/*
��ü�� �浹 �� �������� ��ȣ�ۿ��� �����ϱ� ����
�ʿ��� ���� ������
*/
struct ContactInfo
{
	XMFLOAT3 contactNormal = { 0.0f, 0.0f, 0.0f };
	float penetration = 0.0f;
	XMFLOAT3 contactPoint = { 0.0f, 0.0f, 0.0f };

	// �浹 ����� ���� obj1->GetPosition() - obj2->GetPosition()����
	// �������� obj1�� obj2�� ���� �ڹٲ� ��� ��ֹ��⵵ �ٲپ� �־�� �Ѵ�.
	float normalDirection = 1.0f;
};

/*
��ü���� ���� ��ȣ�ۿ��� ������ Ŭ����
*/
class Physics
{
public:
	static inline XMFLOAT3 gravity = { 0.0f, -9.8f, 0.0f };

public:
	// ��ü�� ��ü ������ �浹�� Ȯ���Ѵ�.
	static bool IsCollision(GameObject* obj1, GameObject* obj2);
	// ��ü�� ���� ������ �浹�� Ȯ���Ѵ�.
	static bool IsCollision(GameObject* obj, const XMVECTOR& rayOrigin, const XMVECTOR& rayDir,
		float& dist, bool isMeshCollision = false);

	// ��ü ������ �浹 �� �ʿ��� ���� �������� ��ȯ�Ѵ�.
	static struct ContactInfo Contact(GameObject* obj1, GameObject* obj2);
	// ��ü�� �浹�Ͽ��� ��, ���� ��ȣ�ۿ��� �����Ѵ�.
	static void Collide(GameObject* obj1, GameObject* obj2, const float deltaTime);

	// ��ü�� ���� �浹�Ͽ��� ��, ��ü�� �ӷ��� �����Ͽ� ƨ�⵵�� �Ѵ�.
	static void ResolveVelocity(GameObject* obj1, GameObject* obj2, const float deltaTime, const ContactInfo& contactInfo);
	// ��ü�� ���� ����Ǿ��� ��, ��ü�� ��ġ�� �����Ͽ� ������ �ذ��Ѵ�.
	static void ResolveInterpenetration(GameObject* obj1, GameObject* obj2, const float deltaTime, const ContactInfo& contactInfo);

	// �� �ٿ�������� �浹�� Ȯ���ϰ� ���� ������ ��ȯ�Ѵ�.
	static ContactInfo ContactSphereAndSphere(GameObject* obj1, GameObject* obj2);
	static ContactInfo ContactBoxAndSphere(GameObject* obj1, GameObject* obj2);
	static ContactInfo ContactAabbAndAabb(GameObject* obj1, GameObject* obj2);
	static ContactInfo ContactBoxAndBox(GameObject* obj1, GameObject* obj2);

	// ��ü�� �ٿ���� aabb���� ������ ���ԵǾ� �������� Ȯ���Ѵ�.
	static bool Contain(GameObject* obj, const BoundingBox& aabb);

};