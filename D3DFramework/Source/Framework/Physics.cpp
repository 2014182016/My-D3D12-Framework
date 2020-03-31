#include "../PrecompiledHeader/pch.h"
#include "Physics.h"
#include "../Component/Mesh.h"
#include "../Object/GameObject.h"

/*
�ش� ��ü�� AABB Ȥ�� OBB�� ��� 
�ٿ�� �ڽ��� Extents�� ��ȯ�Ѵ�.
*/
XMFLOAT3 GetBoxExtents(GameObject* obj)
{
	CollisionType collisionType = obj->GetCollisionType();
	XMFLOAT3 extents = { 0.0f, 0.0f, 0.0f };

	if (collisionType == CollisionType::AABB)
	{
		const BoundingBox& aabb = std::any_cast<BoundingBox>(obj->GetCollisionBounding());
		extents = aabb.Extents;
	}
	else if (collisionType == CollisionType::OBB)
	{
		const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(obj->GetCollisionBounding());
		extents = obb.Extents;
	}

	return extents;
}

/*
��ü�� �߽��� �ڽ��� �������� �վ��ٸ�
�߽ɰ� �ڽ����� ���� ����� ���� ��ȯ�Ѵ�.
*/
void GetClosestPointInBox(const XMFLOAT3& center, const XMFLOAT3& extents, XMFLOAT3& cloestPoint)
{
	// �߽� ��ǥ�� �ڽ��� extents�� ���� ���Ѵ�.
	float x = abs(extents.x - abs(center.x));
	float y = abs(extents.y - abs(center.y));
	float z = abs(extents.z - abs(center.z));

	// ������ ���� ���� ���� ������ ���� �����Ѵ�.
	if (x < y && x < z)
	{
		cloestPoint.x = center.x < 0.0f ? -extents.x : extents.x;
	}
	else if(y < x && y < z)
	{
		cloestPoint.y = center.y < 0.0f ? -extents.y : extents.y;
	}
	else
	{
		cloestPoint.z = center.z < 0.0f ? -extents.z : extents.z;
	}

}

/*
��ü���� �ش� ������ �����Ͽ��� ���� ���̸� ��ȯ�Ѵ�.
*/
float TransformToAxis(GameObject* obj, const XMVECTOR& axis)
{
	// ��ü�� ���� ���Ѵ�.
	XMFLOAT3 extents = GetBoxExtents(obj);
	XMVECTOR boxXAxis = obj->GetAxis(0);
	XMVECTOR boxYAxis = obj->GetAxis(1);
	XMVECTOR boxZAxis = obj->GetAxis(2);

	// ��ü�� �� ��� �ش� ����� ������ �� ����� ������ ��Ÿ����.
	float xAmount = Vector3::DotProduct(axis, boxXAxis);
	float yAmount = Vector3::DotProduct(axis, boxYAxis);
	float zAmount = Vector3::DotProduct(axis, boxZAxis);

	// �����Ͽ� ��ģ��ŭ�� ������ ��ȯ�Ѵ�.
	return extents.x * abs(xAmount) + extents.y * abs(yAmount) + extents.z * abs(zAmount);
}

/*
�� ��ü�� �ش� �࿡ ��ġ�� ������ ��ȯ�Ѵ�.
*/
float OverlapOnAxis(GameObject* obj1, GameObject* obj2, const XMVECTOR& axis, const XMFLOAT3& toCenter)
{
	// ��ü�� ����� ��ġ�� ������ ���Ѵ�.
	float oneProject = TransformToAxis(obj1, axis);
	float twoProject = TransformToAxis(obj2, axis);

	// �߽������� ���Ϳ� ����� ��ġ�� ������ ���Ѵ�.
	XMVECTOR vecToCenter = XMLoadFloat3(&toCenter);
	float distance = abs(Vector3::DotProduct(vecToCenter, axis));

	// 0�����̸� ��ġ�� �ʴ´�, 0�ʰ��̸� ��ģ��.
	return oneProject + twoProject - distance;
}

/*
�� ��ü�� �࿡ ���� ��ġ�� �� ���θ� ��ȯ�ϰ�
�ռ� ����ߴ� ���밪�� �ε��� ���� �̹� ��ġ�� �ͺ���
�� ũ�ٸ� ���� ������ ��ü�Ѵ�.
*/
bool OverlapBoxAxis(GameObject* obj1, GameObject* obj2, const XMVECTOR& axis, const XMFLOAT3& toCenter, const int index,
	float& smallestPenetration, int& smallestIndex)
{
	XMVECTOR axisNormal = XMVector3Normalize(axis);

	float penetration = OverlapOnAxis(obj1, obj2, axisNormal, toCenter);
	if (penetration <= 0.0f)
		return false;

	// �� ��ü�� ��ġ�� ������ ���� ���� ������ �����Ѵ�.
	if (penetration < smallestPenetration)
	{
		smallestPenetration = penetration;
		smallestIndex = index;
	}

	return true;
}

/*
�־��� �� ���� �����ϰ� �࿡ ���� ��ġ�� �� Ȯ���Ѵ�.
*/
bool OverlapBoxAxis(GameObject* obj1, GameObject* obj2, const XMVECTOR& axis1, const XMVECTOR& axis2,
	const XMFLOAT3& toCenter, const int index, float& smallestPenetration, int& smallestIndex)
{
	XMVECTOR axis = XMVector3Cross(axis1, axis2);

	return OverlapBoxAxis(obj1, obj2, axis, toCenter, index, smallestPenetration, smallestIndex);
}

#define CHECK_OVERLAP(axis, index)\
	if(!OverlapBoxAxis(obj1, obj2, axis, toCenter, index, smallestPenetration, smallestIndex)) return contactInfo;

#define CHECK_OVERLAP_CROSS(axis1, axis2, index)\
	if(!OverlapBoxAxis(obj1, obj2, axis1, axis2, toCenter, index, smallestPenetration, smallestIndex)) return contactInfo;

/*
�� ��ü�� �ڽ��̰�, �ڽ����� �ε����� ��
��� �鿡 �ε������� �Ǵ��ϰ� ���� ������ ��ȯ�Ѵ�.
*/
ContactInfo ContactFaceAxisInBox(GameObject* obj1, GameObject* obj2, const XMFLOAT3& toCenter,
	float smallestPenetration, int smallestIndex)
{
	ContactInfo contactInfo;

	XMVECTOR vecToCenter = XMLoadFloat3(&toCenter);
	XMVECTOR normal = obj1->GetAxis(smallestIndex);

	if (Vector3::DotProduct(vecToCenter, normal) > 0.0f)
		normal *=-1.0f;

	XMFLOAT3 vertex = GetBoxExtents(obj2);
	if (Vector3::DotProduct(obj2->GetAxis(0), normal) < 0.0f) vertex.x *= -1.0f;
	if (Vector3::DotProduct(obj2->GetAxis(1), normal) < 0.0f) vertex.y *= -1.0f;
	if (Vector3::DotProduct(obj2->GetAxis(2), normal) < 0.0f) vertex.z *= -1.0f;

	contactInfo.contactNormal = Vector3::XMVectorToFloat3(normal);
	contactInfo.penetration = smallestPenetration;
	contactInfo.contactPoint = obj2->TransformLocalToWorld(vertex);

	return contactInfo;
}

bool Physics::IsCollision(GameObject* obj1, GameObject* obj2)
{
	CollisionType obj1CollisionType = obj1->GetCollisionType();
	CollisionType obj2CollisionType = obj2->GetCollisionType();
	auto obj1Bounding = obj1->GetCollisionBounding();
	auto obj2Bounding = obj2->GetCollisionBounding();

	switch (obj1CollisionType)
	{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb1 = std::any_cast<BoundingBox>(obj1Bounding);
			switch (obj2CollisionType)
			{
				case CollisionType::AABB:
				{
					const BoundingBox& aabb2 = std::any_cast<BoundingBox>(obj2Bounding);
					if (aabb1.Contains(aabb2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
				case CollisionType::OBB:
				{
					const BoundingOrientedBox& obb2 = std::any_cast<BoundingOrientedBox>(obj2Bounding);
					if (aabb1.Contains(obb2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
				case CollisionType::Sphere:
				{
					const BoundingSphere& sphere2 = std::any_cast<BoundingSphere>(obj2Bounding);
					if (aabb1.Contains(sphere2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
			}
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb1 = std::any_cast<BoundingOrientedBox>(obj1Bounding);
			switch (obj2CollisionType)
			{
				case CollisionType::AABB:
				{
					const BoundingBox& aabb2 = std::any_cast<BoundingBox>(obj2Bounding);
					if (obb1.Contains(aabb2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
				case CollisionType::OBB:
				{
					const BoundingOrientedBox& obb2 = std::any_cast<BoundingOrientedBox>(obj2Bounding);
					if (obb1.Contains(obb2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
				case CollisionType::Sphere:
				{
					const BoundingSphere& sphere2 = std::any_cast<BoundingSphere>(obj2Bounding);
					if (obb1.Contains(sphere2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
			}
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere1 = std::any_cast<BoundingSphere>(obj1Bounding);
			switch (obj2CollisionType)
			{
				case CollisionType::AABB:
				{
					const BoundingBox& aabb2 = std::any_cast<BoundingBox>(obj2Bounding);
					if (sphere1.Contains(aabb2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
				case CollisionType::OBB:
				{
					const BoundingOrientedBox& obb2 = std::any_cast<BoundingOrientedBox>(obj2Bounding);
					if (sphere1.Contains(obb2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
				case CollisionType::Sphere:
				{
					const BoundingSphere& sphere2 = std::any_cast<BoundingSphere>(obj2Bounding);
					if (sphere1.Contains(sphere2) != ContainmentType::DISJOINT)
						return true;
					break;
				}
			}
			break;
		}
	}

	return false;
}

bool Physics::IsCollision(class GameObject* obj, const XMVECTOR& rayOrigin, const XMVECTOR& rayDir, float& dist, bool isMeshCollision)
{
	bool isHit = false;

	CollisionType collisionType;
	if (isMeshCollision)
	{
		collisionType = obj->GetMeshCollisionType();
	}
	else
	{
		collisionType = obj->GetCollisionType();
	}

	auto objBounding = obj->GetCollisionBounding();
	switch (collisionType)
	{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(objBounding);
			isHit = aabb.Intersects(rayOrigin, rayDir, dist);
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(objBounding);
			isHit = obb.Intersects(rayOrigin, rayDir, dist);
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(objBounding);
			isHit = sphere.Intersects(rayOrigin, rayDir, dist);
			break;
		}
	}

	return isHit;
}

void Physics::Collide(GameObject* obj1, GameObject* obj2, const float deltaTime)
{
	ContactInfo contactInfo = Contact(obj1, obj2);
	ResolveVelocity(obj1, obj2, deltaTime, contactInfo);
	ResolveInterpenetration(obj1, obj2, deltaTime, contactInfo);
}

ContactInfo Physics::Contact(GameObject* obj1, GameObject* obj2)
{
	ContactInfo contactInfo;

	CollisionType obj1CollisionType = obj1->GetCollisionType();
	CollisionType obj2CollisionType = obj2->GetCollisionType();

	auto obj1Bounding = obj1->GetCollisionBounding();
	auto obj2Bounding = obj2->GetCollisionBounding();

	switch (obj1CollisionType)
	{
		case CollisionType::AABB:
		{
			switch (obj2CollisionType)
			{
				case CollisionType::AABB:
				{
					contactInfo = ContactAabbAndAabb(obj1, obj2);
					contactInfo.normalDirection *= -1.0f;
					return contactInfo;
				}
				case CollisionType::OBB:
				{
					return ContactBoxAndBox(obj1, obj2);
				}
				case CollisionType::Sphere:
				{
					return ContactBoxAndSphere(obj1, obj2);
				}
			}
			break;
		}
		case CollisionType::OBB:
		{
			switch (obj2CollisionType)
			{
				case CollisionType::AABB:
				{
					return ContactBoxAndBox(obj1, obj2);
				}
				case CollisionType::OBB:
				{
					return ContactBoxAndBox(obj1, obj2);
				}
				case CollisionType::Sphere:
				{
					return ContactBoxAndSphere(obj1, obj2);
				}
			}
			break;
		}
		case CollisionType::Sphere:
		{
			switch (obj2CollisionType)
			{
				case CollisionType::AABB:
				{
					contactInfo = ContactBoxAndSphere(obj2, obj1);
					contactInfo.normalDirection *= -1.0f;
					return contactInfo;
				}
				case CollisionType::OBB:
				{
					contactInfo = ContactBoxAndSphere(obj2, obj1);
					contactInfo.normalDirection *= -1.0f;
					return contactInfo;
				}
				case CollisionType::Sphere: 
				{
					return ContactSphereAndSphere(obj1, obj2);
				}
			}
			break;
		}
	}

	return contactInfo;
}

ContactInfo Physics::ContactSphereAndSphere(GameObject* obj1, GameObject* obj2)
{
	ContactInfo contactInfo;

	XMFLOAT3 pos1 = obj1->GetPosition();
	XMFLOAT3 pos2 = obj2->GetPosition();

	float radius1 = std::any_cast<BoundingSphere>(obj1->GetCollisionBounding()).Radius;
	float radius2 = std::any_cast<BoundingSphere>(obj2->GetCollisionBounding()).Radius;

	// �� ��ü ��ġ ������ ���� �߽������� ���͸� ���Ѵ�.
	XMFLOAT3 midLine = Vector3::Subtract(pos1, pos2);
	float size = Vector3::Length(midLine);

	// �߽� ���Ͱ� �� ��ü�� ������ �պ��� �۴ٸ� �浹���� �ʴ´�.
	if (size <= 0.0f || size >= radius1 + radius2)
		return contactInfo;

	contactInfo.contactNormal = Vector3::Multiply(midLine, 1.0f / size);
	contactInfo.contactPoint = Vector3::Add(pos1, Vector3::Multiply(midLine, 0.5f));
	contactInfo.penetration = radius1 + radius2 - size;

	return contactInfo;
}

ContactInfo Physics::ContactBoxAndSphere(class GameObject* obj1, class GameObject* obj2)
{
	ContactInfo contactInfo;

	const BoundingSphere& sphere = std::any_cast<BoundingSphere>(obj2->GetCollisionBounding());
	XMFLOAT3 extents = GetBoxExtents(obj1);

	// ���� �߽� ��ǥ�� �ڽ��� ���� ��ǥ��� ��ȯ�Ѵ�.
	// �̷��� �����ν� �ڽ��� ���� �浹�� �ܼ�ȭ��ų �� �ִ�.
	XMFLOAT3 sphereCenter = obj1->TransformWorldToLocal(obj2->GetPosition());
	XMFLOAT3 cloestPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);

	cloestPoint.x = std::clamp(sphereCenter.x, -extents.x, extents.x);
	cloestPoint.y = std::clamp(sphereCenter.y, -extents.y, extents.y);
	cloestPoint.z = std::clamp(sphereCenter.z, -extents.z, extents.z);

	if (Vector3::Equal(cloestPoint, sphereCenter))
	{
		GetClosestPointInBox(sphereCenter, extents, cloestPoint);
		// ���� �߽��� �ڽ� �ȿ� �ִٸ�
		// �浹 ����� ������ �ٲ��.
		contactInfo.normalDirection *= -1.0f;
	}

	float distance = Vector3::Length(Vector3::Subtract(cloestPoint, sphereCenter));
	// �浹���� ���� ����� ������ ���� �߽��� ���������� ũ�ٸ� �浹�� ���� �ʴ´�.
	if (distance > sphere.Radius * sphere.Radius)
		return contactInfo;

	// ���� ����� ������ ���� ��ǥ��� ��ȯ�Ѵ�.
	cloestPoint = obj1->TransformLocalToWorld(cloestPoint);

	contactInfo.contactNormal = Vector3::Normalize(Vector3::Subtract(cloestPoint, obj2->GetPosition()));
	contactInfo.contactPoint = cloestPoint;
	contactInfo.penetration = sphere.Radius - sqrt(distance);

	return contactInfo;
}

ContactInfo Physics::ContactAabbAndAabb(GameObject* obj1, GameObject* obj2)
{
	ContactInfo contactInfo;

	XMFLOAT3 toCenter = Vector3::Subtract(obj1->GetPosition(), obj2->GetPosition());
	float smallestPenetration = FLT_MAX;
	int smallestIndex = -1;

	// AABB�� X��, Y��, Z����� �浹�� Ȯ���ϸ� �ȴ�.
	CHECK_OVERLAP(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0);
	CHECK_OVERLAP(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 1);
	CHECK_OVERLAP(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), 2);

	return ContactFaceAxisInBox(obj1, obj2, toCenter, smallestPenetration, smallestIndex);
}

ContactInfo Physics::ContactBoxAndBox(GameObject* obj1, GameObject* obj2)
{
	ContactInfo contactInfo;

	XMFLOAT3 toCenter = Vector3::Subtract(obj1->GetPosition(), obj2->GetPosition());
	float smallestPenetration = FLT_MAX;
	int smallestIndex = -1;

	XMVECTOR obj1Axis0 = obj1->GetAxis(0);
	XMVECTOR obj1Axis1 = obj1->GetAxis(1);
	XMVECTOR obj1Axis2 = obj1->GetAxis(2);

	XMVECTOR obj2Axis0 = obj2->GetAxis(0);
	XMVECTOR obj2Axis1 = obj2->GetAxis(1);
	XMVECTOR obj2Axis2 = obj2->GetAxis(2);

	CHECK_OVERLAP(obj1Axis0, 0);
	CHECK_OVERLAP(obj1Axis1, 1);
	CHECK_OVERLAP(obj1Axis2, 2);

	CHECK_OVERLAP(obj2Axis0, 3);
	CHECK_OVERLAP(obj2Axis1, 4);
	CHECK_OVERLAP(obj2Axis2, 5);

	CHECK_OVERLAP_CROSS(obj1Axis0, obj2Axis0, 6);
	CHECK_OVERLAP_CROSS(obj1Axis0, obj2Axis1, 7);
	CHECK_OVERLAP_CROSS(obj1Axis0, obj2Axis2, 8);
	CHECK_OVERLAP_CROSS(obj1Axis1, obj2Axis0, 9);
	CHECK_OVERLAP_CROSS(obj1Axis1, obj2Axis1, 10);
	CHECK_OVERLAP_CROSS(obj1Axis1, obj2Axis2, 11);
	CHECK_OVERLAP_CROSS(obj1Axis2, obj2Axis0, 12);
	CHECK_OVERLAP_CROSS(obj1Axis2, obj2Axis1, 13);
	CHECK_OVERLAP_CROSS(obj1Axis2, obj2Axis2, 14);

	if (smallestIndex < 3)
	{
		return ContactFaceAxisInBox(obj1, obj2, toCenter, smallestPenetration, smallestIndex);
	}
	else if (smallestIndex < 6)
	{
		return ContactFaceAxisInBox(obj2, obj1, Vector3::Multiply(toCenter, -1.0f), smallestPenetration, smallestIndex - 3);
	}
	else
	{

	}

	return contactInfo;
}

void Physics::ResolveVelocity(GameObject* obj1, GameObject* obj2, const float deltaTime, const ContactInfo& contactInfo)
{
	XMFLOAT3 velDif = Vector3::Subtract(obj1->GetVelocity(), obj2->GetVelocity());
	XMFLOAT3 contactNormal = Vector3::Normalize(Vector3::Multiply(contactInfo.contactNormal, contactInfo.normalDirection));

	// �� ��ü�� ���� ���⿡ ���� �и��ӵ��� ����Ѵ�.
	float separatingVelocity = Vector3::DotProduct(velDif, contactNormal);

	// ������ �Ͼ���� ��ü�� �и��ǰ� �ְų�
	// ������ ���� ��� ��ݷ��� �ʿ����.
	if (separatingVelocity >= 0.0f)
		return;

	// �浹 �� �ݹ� ����� �� ��ü �� �ּ��� �ݹ� ����� ��´�.
	float restitution = std::min<float>(obj1->restitution, obj2->restitution);

	// ���Ӱ� ����� �и� �ӵ�
	float newSepVel = -separatingVelocity * restitution;
	// �ӵ��� ���ӵ��� ���� �������� �˻��Ѵ�.
	XMFLOAT3 accDif = Vector3::Subtract(obj1->GetAcceleration(), obj2->GetAcceleration());
	float accCausedSepVel = Vector3::DotProduct(accDif, contactNormal) * deltaTime;

	// ���ӵ��� ���� ���� �ӵ��� ��������
	// �̸� ���ο� �и� �ӵ����� �����Ѵ�.
	if (accCausedSepVel <= 0.0f)
	{
		newSepVel += accCausedSepVel * restitution;

		// ���� �ʿ��� �ͺ��� �� ���� ���´��� Ȯ���Ѵ�.
		if (newSepVel <= 0.0f)
			newSepVel = 0.0f;
	}

	float deltaVel = newSepVel - separatingVelocity;

	// ������ ��ü�� ���Ͽ� ������ �ݺ���Ͽ� �ӵ��� �����Ѵ�.
	// ������ ���� ��ü�ϼ��� �ӵ��� ��ȭ�� ���ϴ�.
	float totalInvMass = obj1->GetInvMass() + obj2->GetInvMass();

	// ��� ��ü�� ������ ���Ѵ��̸� ��ݷ��� ȿ���� ����.
	if (totalInvMass <= 0)
		return;

	// ������ ��ݷ��� ����Ѵ�.
	float impulseScale = deltaVel / totalInvMass;
	XMFLOAT3 impulse = Vector3::Multiply(contactNormal, impulseScale);
	XMFLOAT3 impulseOpp = Vector3::Multiply(impulse, -1.0f);

	// ��ݷ��� �����Ѵ�.
	obj1->Impulse(impulse);
	// �ݴ�������� ��ݷ��� �����Ѵ�.
	obj2->Impulse(impulseOpp);
}

void Physics::ResolveInterpenetration(GameObject* obj1, GameObject* obj2, const float deltaTime, const ContactInfo& contactInfo)
{
	static const float slop = 0.01f;

	// ������ �κ��� ������ �ǳʶڴ�.
	if (contactInfo.penetration <= 0.0f)
		return;

	// ��ü�� �Ű��ִ� �Ÿ��� ������ �ݺ���ϹǷ� ������ �� ���Ѵ�.
	float totalInvMass = obj1->GetInvMass() + obj2->GetInvMass();

	// ��� ��ü�� ������ ���Ѵ��̸� �ƹ��� ó���� ���� �ʴ´�.
	if (totalInvMass <= 0.0f)
		return;

	XMFLOAT3 contactNormal = Vector3::Normalize(Vector3::Multiply(contactInfo.contactNormal, contactInfo.normalDirection));

	// ��ü�� �Űܰ� �Ÿ��� ������ �ݺ���Ѵ�.
	// ��ü�� �ٸ� ��ü�� �������� ��, �����ϴ� ���� ���� ���� slop�� �������ش�.
	float move = std::max<float>(contactInfo.penetration - slop, 0.0f) / totalInvMass;
	XMFLOAT3 movePerInvMass = Vector3::Multiply(contactNormal, move);
	XMFLOAT3 movement1 = Vector3::Multiply(movePerInvMass, obj1->GetInvMass());
	XMFLOAT3 movement2 = Vector3::Multiply(movePerInvMass, obj2->GetInvMass() * -1.0f);

	// ��ü�� �ű��.
	obj1->Move(movement1);
	// �ݴ�������� �ű��.
	obj2->Move(movement2);
}

bool Physics::Contain(GameObject* obj, const BoundingBox& aabb)
{
	auto objBounding = obj->GetCollisionBounding();
	switch (obj->GetCollisionType())
	{
		case CollisionType::AABB:
		{
			const BoundingBox& objAabb = std::any_cast<BoundingBox>(objBounding);
			if (aabb.Contains(objAabb) == ContainmentType::CONTAINS)
				return true;
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(objBounding);
			if (aabb.Contains(obb) == ContainmentType::CONTAINS)
				return true;
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(objBounding);
			if (aabb.Contains(sphere) == ContainmentType::CONTAINS)
				return true;
			break;
		}
	}

	return false;
}