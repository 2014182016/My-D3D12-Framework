#include "../PrecompiledHeader/pch.h"
#include "Physics.h"
#include "../Component/Mesh.h"
#include "../Object/GameObject.h"

/*
해당 객체가 AABB 혹은 OBB일 경우 
바운딩 박스의 Extents를 반환한다.
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
객체의 중심이 박스의 안쪽으로 뚫었다면
중심과 박스에서 가장 가까운 점을 반환한다.
*/
void GetClosestPointInBox(const XMFLOAT3& center, const XMFLOAT3& extents, XMFLOAT3& cloestPoint)
{
	// 중심 좌표와 박스의 extents의 차를 구한다.
	float x = abs(extents.x - abs(center.x));
	float y = abs(extents.y - abs(center.y));
	float z = abs(extents.z - abs(center.z));

	// 차이의 값이 가장 작은 곳으로 점을 투영한다.
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
객체에서 해당 축으로 투영하였을 때의 길이를 반환한다.
*/
float TransformToAxis(GameObject* obj, const XMVECTOR& axis)
{
	// 객체의 축을 구한다.
	XMFLOAT3 extents = GetBoxExtents(obj);
	XMVECTOR boxXAxis = obj->GetAxis(0);
	XMVECTOR boxYAxis = obj->GetAxis(1);
	XMVECTOR boxZAxis = obj->GetAxis(2);

	// 객체의 각 축과 해당 축과의 내적은 각 축과의 정도를 나타낸다.
	float xAmount = Vector3::DotProduct(axis, boxXAxis);
	float yAmount = Vector3::DotProduct(axis, boxYAxis);
	float zAmount = Vector3::DotProduct(axis, boxZAxis);

	// 투영하여 겹친만큼의 길이을 반환한다.
	return extents.x * abs(xAmount) + extents.y * abs(yAmount) + extents.z * abs(zAmount);
}

/*
두 객체가 해당 축에 겹치는 정도를 반환한다.
*/
float OverlapOnAxis(GameObject* obj1, GameObject* obj2, const XMVECTOR& axis, const XMFLOAT3& toCenter)
{
	// 객체와 축과의 겹치는 정도를 구한다.
	float oneProject = TransformToAxis(obj1, axis);
	float twoProject = TransformToAxis(obj2, axis);

	// 중심으로의 벡터와 축과의 겹치는 정도를 구한다.
	XMVECTOR vecToCenter = XMLoadFloat3(&toCenter);
	float distance = abs(Vector3::DotProduct(vecToCenter, axis));

	// 0이하이면 겹치지 않는다, 0초과이면 겹친다.
	return oneProject + twoProject - distance;
}

/*
두 객체가 축에 대해 겹치는 지 여부를 반환하고
앞서 계산했던 관통값과 인덱스 값이 이번 겹치는 것보다
더 크다면 작은 값으로 대체한다.
*/
bool OverlapBoxAxis(GameObject* obj1, GameObject* obj2, const XMVECTOR& axis, const XMFLOAT3& toCenter, const int index,
	float& smallestPenetration, int& smallestIndex)
{
	XMVECTOR axisNormal = XMVector3Normalize(axis);

	float penetration = OverlapOnAxis(obj1, obj2, axisNormal, toCenter);
	if (penetration <= 0.0f)
		return false;

	// 두 객체의 겹치는 정도가 가장 작은 값들을 저장한다.
	if (penetration < smallestPenetration)
	{
		smallestPenetration = penetration;
		smallestIndex = index;
	}

	return true;
}

/*
주어진 두 축을 외적하고 축에 대해 겹치는 지 확인한다.
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
두 객체는 박스이고, 박스끼리 부딪혔을 때
어느 면에 부딪혔는지 판단하고 물리 정보를 반환한다.
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

	// 두 객체 위치 벡터의 차로 중심으로의 벡터를 구한다.
	XMFLOAT3 midLine = Vector3::Subtract(pos1, pos2);
	float size = Vector3::Length(midLine);

	// 중심 벡터가 두 객체의 반지름 합보다 작다면 충돌하지 않는다.
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

	// 구의 중심 좌표를 박스의 로컬 좌표계로 변환한다.
	// 이렇게 함으로써 박스와 구의 충돌을 단순화시킬 수 있다.
	XMFLOAT3 sphereCenter = obj1->TransformWorldToLocal(obj2->GetPosition());
	XMFLOAT3 cloestPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);

	cloestPoint.x = std::clamp(sphereCenter.x, -extents.x, extents.x);
	cloestPoint.y = std::clamp(sphereCenter.y, -extents.y, extents.y);
	cloestPoint.z = std::clamp(sphereCenter.z, -extents.z, extents.z);

	if (Vector3::Equal(cloestPoint, sphereCenter))
	{
		GetClosestPointInBox(sphereCenter, extents, cloestPoint);
		// 구의 중심이 박스 안에 있다면
		// 충돌 노멀은 방향이 바뀐다.
		contactInfo.normalDirection *= -1.0f;
	}

	float distance = Vector3::Length(Vector3::Subtract(cloestPoint, sphereCenter));
	// 충돌에서 가장 가까운 지점과 구의 중심이 반지름보다 크다면 충돌은 나지 않는다.
	if (distance > sphere.Radius * sphere.Radius)
		return contactInfo;

	// 가장 가까운 지점을 월드 좌표계로 변환한다.
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

	// AABB는 X축, Y축, Z축과의 충돌만 확인하면 된다.
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

	// 두 물체의 접촉 방향에 대한 분리속도를 계산한다.
	float separatingVelocity = Vector3::DotProduct(velDif, contactNormal);

	// 접촉이 일어났으나 물체가 분리되고 있거나
	// 가만히 잇을 경우 충격량이 필요없다.
	if (separatingVelocity >= 0.0f)
		return;

	// 충돌 시 반발 계수는 두 객체 중 최소의 반발 계수로 잡는다.
	float restitution = std::min<float>(obj1->restitution, obj2->restitution);

	// 새롭게 계산한 분리 속도
	float newSepVel = -separatingVelocity * restitution;
	// 속도가 가속도에 의한 것인지를 검사한다.
	XMFLOAT3 accDif = Vector3::Subtract(obj1->GetAcceleration(), obj2->GetAcceleration());
	float accCausedSepVel = Vector3::DotProduct(accDif, contactNormal) * deltaTime;

	// 가속도에 의해 접근 속도가 생겼으면
	// 이를 새로운 분리 속도에서 제거한다.
	if (accCausedSepVel <= 0.0f)
	{
		newSepVel += accCausedSepVel * restitution;

		// 실제 필요한 것보다 더 많이 빼냈는지 확인한다.
		if (newSepVel <= 0.0f)
			newSepVel = 0.0f;
	}

	float deltaVel = newSepVel - separatingVelocity;

	// 각각의 물체에 대하여 질량에 반비례하여 속도를 변경한다.
	// 질량이 작은 물체일수록 속도의 변화가 심하다.
	float totalInvMass = obj1->GetInvMass() + obj2->GetInvMass();

	// 모든 물체의 질량이 무한대이면 충격량에 효과가 없다.
	if (totalInvMass <= 0)
		return;

	// 적용할 충격략을 계산한다.
	float impulseScale = deltaVel / totalInvMass;
	XMFLOAT3 impulse = Vector3::Multiply(contactNormal, impulseScale);
	XMFLOAT3 impulseOpp = Vector3::Multiply(impulse, -1.0f);

	// 충격량을 적용한다.
	obj1->Impulse(impulse);
	// 반대방향으로 충격량을 적용한다.
	obj2->Impulse(impulseOpp);
}

void Physics::ResolveInterpenetration(GameObject* obj1, GameObject* obj2, const float deltaTime, const ContactInfo& contactInfo)
{
	static const float slop = 0.01f;

	// 겹쳐진 부분이 없으면 건너뛴다.
	if (contactInfo.penetration <= 0.0f)
		return;

	// 물체를 옮겨주는 거리는 질량에 반비례하므로 질량을 다 더한다.
	float totalInvMass = obj1->GetInvMass() + obj2->GetInvMass();

	// 모든 물체의 질량이 무한대이면 아무런 처리도 하지 않는다.
	if (totalInvMass <= 0.0f)
		return;

	XMFLOAT3 contactNormal = Vector3::Normalize(Vector3::Multiply(contactInfo.contactNormal, contactInfo.normalDirection));

	// 물체가 옮겨갈 거리는 질량에 반비례한다.
	// 물체가 다른 물체에 놓여있을 때, 진동하는 것을 막기 위해 slop을 적용해준다.
	float move = std::max<float>(contactInfo.penetration - slop, 0.0f) / totalInvMass;
	XMFLOAT3 movePerInvMass = Vector3::Multiply(contactNormal, move);
	XMFLOAT3 movement1 = Vector3::Multiply(movePerInvMass, obj1->GetInvMass());
	XMFLOAT3 movement2 = Vector3::Multiply(movePerInvMass, obj2->GetInvMass() * -1.0f);

	// 물체를 옮긴다.
	obj1->Move(movement1);
	// 반대방향으로 옮긴다.
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