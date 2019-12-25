#include "pch.h"
#include "Octree.h"
#include "GameObject.h"
#include "MeshGeometry.h"
#include "Enums.h"

using namespace DirectX;

Octree::Octree(const DirectX::BoundingBox& boundingBox, const std::list<std::shared_ptr<GameObject>>& objList)
{
	mBoundingBox = boundingBox;

	for (const auto& obj : objList)
	{
		if(obj->GetCollisionType() != CollisionType::None)
			mObjectList.emplace_front(obj);
	}

	for (int i = 0; i < 8; ++i)
		mChildNodes[i] = nullptr;
}

Octree::Octree(const BoundingBox& boundingBox)
{
	mBoundingBox = boundingBox;

	for (int i = 0; i < 8; ++i)
		mChildNodes[i] = nullptr;
}

void Octree::BuildTree()
{
	// 오브젝트가 1개 이하일 경우 BuildTree를 종료한다.
	if (mObjectList.size() <= 1)
		return;

	float dimension = mBoundingBox.Extents.x;

	if (dimension <= MIN_SIZE)
		return;

	float half = dimension / 2.0f;
	XMFLOAT3 center = mBoundingBox.Center;
	XMFLOAT3 extents = mBoundingBox.Extents;

	// 현재 바운딩 박스를 8개의 공간으로 분할
	BoundingBox octant[8];
	octant[0] = BoundingBox((center - extents) / 2.0f, XMFLOAT3(half, half, half));
	octant[1] = BoundingBox((center + XMFLOAT3(-extents.x,-extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[2] = BoundingBox((center + XMFLOAT3(extents.x, -extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[3] = BoundingBox((center + XMFLOAT3(extents.x, -extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[5] = BoundingBox((center + extents) / 2.0f, XMFLOAT3(half, half, half));
	octant[4] = BoundingBox((center + XMFLOAT3(-extents.x, extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[6] = BoundingBox((center + XMFLOAT3(extents.x, extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[7] = BoundingBox((center + XMFLOAT3(-extents.x, extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));

	// 각 8개의 노드의 오브젝트 리스트
	std::list<std::shared_ptr<GameObject>> octList[8];

	// 현재 가지고 있는 오브젝트 리스트로부터 없애야 할 오브젝트들을 담는 리스트
	std::list<std::shared_ptr<GameObject>> delist;

	for (const auto& obj : mObjectList)
	{
		if (!obj->GetMesh())
			continue;

		switch (obj->GetMesh()->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(obj->GetCollisionBounding());
			for (int i = 0; i < 8; ++i)
			{
				if (octant[i].Contains(aabb) == ContainmentType::CONTAINS)
				{
					octList[i].emplace_front(obj);
					delist.emplace_front(obj);
					break;
				}
			}
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(obj->GetCollisionBounding());
			for (int i = 0; i < 8; ++i)
			{
				if (octant[i].Contains(obb) == ContainmentType::CONTAINS)
				{
					octList[i].emplace_front(obj);
					delist.emplace_front(obj);
					break;
				}
			}
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(obj->GetCollisionBounding());
			for (int i = 0; i < 8; ++i)
			{
				if (octant[i].Contains(sphere) == ContainmentType::CONTAINS)
				{
					octList[i].emplace_front(obj);
					delist.emplace_front(obj);
					break;
				}
			}
			break;
		}
		}

	}

	// 하위 노드에 포함된 오브젝트는 이 노드에서부터 삭제한다.
	for (auto& obj : delist)
		mObjectList.remove(obj);

	// 하위 노드에 오브젝트가 존재한다면 BuildTree를 재귀적으로 실행한다.
	for (int i = 0; i < 8; ++i)
	{
		UINT objectCount = (UINT)std::distance(octList[i].begin(), octList[i].end());
		if (objectCount != 0)
		{
			mChildNodes[i] = CreateNode(octant[i], octList[i]);
			mActiveNodes |= (byte)(1 << i);
			mChildNodes[i]->BuildTree();
		}
	}

	mTreeBuilt = true;
	mTreeReady = true;
}

bool Octree::Insert(std::shared_ptr<GameObject> obj)
{
	if (mObjectList.size() == 0)
	{
		mObjectList.emplace_front(obj);
		return true;
	}

	float dimension = mBoundingBox.Extents.x;
	if (dimension <= MIN_SIZE)
	{
		mObjectList.emplace_front(obj);
		return true;
	}

	float half = dimension / 2.0f;
	XMFLOAT3 center = mBoundingBox.Center;
	XMFLOAT3 extents = mBoundingBox.Extents;

	// 현재 바운딩 박스를 8개의 공간으로 분할
	BoundingBox octant[8];
	octant[0] = (mChildNodes[0] != nullptr) ? mChildNodes[0]->GetBoundingBox() : BoundingBox((center - extents) / 2.0f, XMFLOAT3(half, half, half));
	octant[1] = (mChildNodes[1] != nullptr) ? mChildNodes[1]->GetBoundingBox() : BoundingBox((center + XMFLOAT3(-extents.x, -extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[2] = (mChildNodes[2] != nullptr) ? mChildNodes[2]->GetBoundingBox() : BoundingBox((center + XMFLOAT3(extents.x, -extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[3] = (mChildNodes[3] != nullptr) ? mChildNodes[3]->GetBoundingBox() : BoundingBox((center + XMFLOAT3(extents.x, -extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[5] = (mChildNodes[4] != nullptr) ? mChildNodes[4]->GetBoundingBox() : BoundingBox((center + extents) / 2.0f, XMFLOAT3(half, half, half));
	octant[4] = (mChildNodes[5] != nullptr) ? mChildNodes[5]->GetBoundingBox() : BoundingBox((center + XMFLOAT3(-extents.x, extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[6] = (mChildNodes[6] != nullptr) ? mChildNodes[6]->GetBoundingBox() : BoundingBox((center + XMFLOAT3(extents.x, extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[7] = (mChildNodes[7] != nullptr) ? mChildNodes[7]->GetBoundingBox() : BoundingBox((center + XMFLOAT3(-extents.x, extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));

	switch (obj->GetMesh()->GetCollisionType())
	{
	case CollisionType::AABB:
	{
		const BoundingBox& aabb = std::any_cast<BoundingBox>(obj->GetCollisionBounding());
		for (int i = 0; i < 8; ++i)
		{
			if (octant[i].Contains(aabb) == ContainmentType::CONTAINS)
			{
				if (mChildNodes[i] != nullptr)
				{
					return mChildNodes[i]->Insert(obj);
				}
				else
				{
					mChildNodes[i] = CreateNode(octant[i], obj);
					mActiveNodes |= (byte)(1 << i);
					return mChildNodes[i]->Insert(obj);
				}
			}
		}

		mObjectList.emplace_front(obj);
		return true;
	}
	case CollisionType::OBB:
	{
		const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(obj->GetCollisionBounding());
		for (int i = 0; i < 8; ++i)
		{
			if (octant[i].Contains(obb) == ContainmentType::CONTAINS)
			{
				if (mChildNodes[i] != nullptr)
					return mChildNodes[i]->Insert(obj);
				else
				{
					mChildNodes[i] = CreateNode(octant[i], obj);
					mActiveNodes |= (byte)(1 << i);
					return mChildNodes[i]->Insert(obj);
				}
			}
		}

		mObjectList.emplace_front(obj);
		return true;
	}
	case CollisionType::Sphere:
	{
		const BoundingSphere& sphere = std::any_cast<BoundingSphere>(obj->GetCollisionBounding());
		for (int i = 0; i < 8; ++i)
		{
			if (octant[i].Contains(sphere) == ContainmentType::CONTAINS)
			{
				if (mChildNodes[i] != nullptr)
					return mChildNodes[i]->Insert(obj);
				else
				{
					mChildNodes[i] = CreateNode(octant[i], obj);
					mActiveNodes |= (byte)(1 << i);
					return mChildNodes[i]->Insert(obj);
				}
			}
		}

		mObjectList.emplace_front(obj);
		return true;
	}
	}

	return false;
}

void Octree::Update(float deltaTime)
{
	if (mTreeBuilt == false || mTreeReady == false)
		return;

	// 이 노드가 오브젝트를 가지고 있지 않다면 카운트 다운을 시작하고,
	// 카운트 다운이 0이 될 시에 이 노드를 삭제한다.
	// 죽기 전에 노드를 다시 사용하게 됐다면 수명을 두 배로 늘린다.
	// 이로써 불필요한 메모리 할당 및 해제를 피할 수 있다.
	if (mObjectList.size() == 0)
	{
		if (!mHasChildren)
		{
			if (mCurrLife == -1)
				mCurrLife = mMaxLifespan;
			else if (mCurrLife > 0)
				--mCurrLife;
		}
	}
	else
	{
		if (mCurrLife != -1)
		{
			if (mMaxLifespan <= 64)
				mMaxLifespan *= 2;
			mCurrLife = -1;
		}
	}

	std::list<std::shared_ptr<GameObject>> movedObjects;
	for (const auto& obj : mObjectList)
	{
		// 이 함수는 Tick이 불리고 난 이후에 WorldUpdate가 Set되므로
		// Update함수는 Object의 Tick함수 이전에 불려져야 한다.
		if (obj->GetIsMovable() && obj->GetIsWorldUpdate())
			movedObjects.emplace_front(obj);
	}

	mHasChildren = false;
	// CurrLife가 0이 된 노드는 삭제한다.
	for (int flags = mActiveNodes, index = 0; flags > 0; flags >>= 1, ++index)
	{
		if (mChildNodes[index] != nullptr)
		{
			mHasChildren = true;

			if ((flags & 1) == 1 && (mChildNodes[index]->GetCurrentLife() == 0))
			{
				// 오브젝트가 존재할 떄, 노드를 삭제해서는 안된다.
				if (mChildNodes[index]->GetObjectCount() > 0)
				{
					mChildNodes[index]->SetCurrentLife(-1);
				}
				// 오브젝트가 없고, CurrLife = 0이라면 노드를 삭제한다.
				else
				{
					delete mChildNodes[index];
					mChildNodes[index] = nullptr;
					mActiveNodes ^= (byte)(1 << index);
				}
			}
		}
	}

	// 자식 노드들도 업데이트한다.
	for (int flags = mActiveNodes, index = 0; flags > 0; flags >>= 1, index++)
	{
		if ((flags & 1) == 1)
		{
			if (mChildNodes != nullptr && mChildNodes[index] != nullptr)
				mChildNodes[index]->Update(deltaTime);
		}
	}

	// 오브젝트가 움직였다면 부모 노드로 이동하여 알맞는 노드를 다시 찾는다.
	for (auto& obj : movedObjects)
	{
		Octree* currentNode = this;

		// 오브젝트의 바운딩 박스를 완전히 포함할 부모 노드를 계속해서 찾아간다.
		switch (obj->GetMesh()->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(obj->GetCollisionBounding());
			while (currentNode->GetBoundingBox().Contains(aabb) != ContainmentType::CONTAINS)
			{
				if (currentNode->GetParent() != nullptr)
					currentNode = currentNode->GetParent();
				else
					break;
			}
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(obj->GetCollisionBounding());
			while (currentNode->GetBoundingBox().Contains(obb) != ContainmentType::CONTAINS)
			{
				if (currentNode->GetParent() != nullptr)
					currentNode = currentNode->GetParent();
				else
					break;
			}
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(obj->GetCollisionBounding());
			while (currentNode->GetBoundingBox().Contains(sphere) != ContainmentType::CONTAINS)
			{
				if (currentNode->GetParent() != nullptr)
					currentNode = currentNode->GetParent();
				else
					break;
			}
			break;
		}
		}
	
		if (currentNode == this)
			continue;

		// 현재 노드에서 움직인 오브젝트를 삭제하고,
		// 알맞은 노드에 다시 삽입한다.
		currentNode->Insert(obj);
		mObjectList.remove(obj);
	}

	// 부모노드에 있는 오브젝트들과 1:1검사를 한 뒤
	// 현재 노드에 있는 오브젝트끼리 충돌검사를 한다.
	std::list<std::shared_ptr<GameObject>> currentObjList, parentObjList;
	currentObjList = mObjectList;
	GetParentObjectList(parentObjList);

	while (!parentObjList.empty())
	{
		std::shared_ptr<GameObject> obj = parentObjList.front();
		parentObjList.pop_front();

		for (auto& other : currentObjList)
		{
			bool isCollision = obj->IsCollision(other);
			if (isCollision)
			{
				obj->Collide(other);
			}
		}
	}

	while (!currentObjList.empty())
	{
		std::shared_ptr<GameObject> obj = currentObjList.front();
		currentObjList.pop_front();

		for (auto& other : currentObjList)
		{
			bool isCollision = obj->IsCollision(other);
			if (isCollision)
			{
				obj->Collide(other);
			}
		}
	}
}


Octree* Octree::CreateNode(const BoundingBox& boundingBox, std::list<std::shared_ptr<GameObject>> objList)
{
	UINT objCount = (UINT)std::distance(objList.begin(), objList.end());
	if (objCount == 0)
		return nullptr;

	Octree* newOctant = new Octree(boundingBox, objList);
	newOctant->SetParent(this);
	return newOctant;
}

Octree* Octree::CreateNode(const BoundingBox& boundingBox, std::shared_ptr<GameObject> obj)
{
	std::list<std::shared_ptr<GameObject>> objList;
	objList.emplace_front(obj);

	Octree* newOctant = new Octree(boundingBox, objList);
	newOctant->SetParent(this);
	return newOctant;
}

void Octree::GetBoundingWorlds(std::vector<XMFLOAT4X4>& worlds) const
{
	XMMATRIX translation = XMMatrixTranslation(mBoundingBox.Center.x, mBoundingBox.Center.y, mBoundingBox.Center.z);
	XMMATRIX scailing = XMMatrixScaling(mBoundingBox.Extents.x, mBoundingBox.Extents.y, mBoundingBox.Extents.z);
	XMMATRIX world = XMMatrixMultiply(scailing, translation);
	XMFLOAT4X4 world4x4f;
	XMStoreFloat4x4(&world4x4f, XMMatrixTranspose(world));
	worlds.push_back(std::move(world4x4f));

	for (int i = 0; i < 8; ++i)
	{
		if (mChildNodes[i] != nullptr)
			mChildNodes[i]->GetBoundingWorlds(worlds);
	}
}

void Octree::GetIntersectObjects(const DirectX::BoundingBox& bounding, std::list<std::shared_ptr<GameObject>>& objects)
{
	ContainmentType containmentType = mBoundingBox.Contains(bounding);

	switch (containmentType)
	{
	case DirectX::DISJOINT:
		break;
	case DirectX::INTERSECTS:
		objects.insert(objects.end(), mObjectList.begin(), mObjectList.end());

		for (int i = 0; i < 8; ++i)
		{
			if (mChildNodes[i] != nullptr)
			{
				mChildNodes[i]->GetIntersectObjects(bounding, objects);
			}
		}
		break;
	case DirectX::CONTAINS:
		GetChildObjectList(objects);
		break;
	default:
		break;
	}
}

void Octree::GetParentObjectList(std::list<std::shared_ptr<GameObject>>& objList) const
{
	Octree* parentNode = mParent;
	while (parentNode != nullptr)
	{
		const std::list<std::shared_ptr<GameObject>>& parentObjList = parentNode->GetObjectList();
		objList.insert(objList.end(), parentObjList.begin(), parentObjList.end());
		parentNode = parentNode->GetParent();
	}
}

void Octree::GetChildObjectList(std::list<std::shared_ptr<class GameObject>>& objList) const
{
	if (mChildNodes == nullptr)
		return;

	for (int i = 0; i < 8; ++i)
	{
		if (mChildNodes[i] != nullptr)
		{
			std::list<std::shared_ptr<class GameObject>> chileObjList = mChildNodes[i]->GetObjectList();
			objList.insert(objList.end(), chileObjList.begin(), chileObjList.end());
			mChildNodes[i]->GetChildObjectList(objList);
		}
	}
}

void Octree::DestroyObjects()
{
	mObjectList.remove_if([](std::shared_ptr<GameObject>& obj)->bool
	{ return obj->GetIsDestroyesd(); });

	if (mChildNodes == nullptr)
		return;

	for (int i = 0; i < 8; ++i)
	{
		if (mChildNodes[i] != nullptr)
		{
			mChildNodes[i]->DestroyObjects();
		}
	}
}