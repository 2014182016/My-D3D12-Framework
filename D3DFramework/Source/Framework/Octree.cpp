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
	// ������Ʈ�� 1�� ������ ��� BuildTree�� �����Ѵ�.
	if (mObjectList.size() <= 1)
		return;

	float dimension = mBoundingBox.Extents.x;

	if (dimension <= MIN_SIZE)
		return;

	float half = dimension / 2.0f;
	XMFLOAT3 center = mBoundingBox.Center;
	XMFLOAT3 extents = mBoundingBox.Extents;

	// ���� �ٿ�� �ڽ��� 8���� �������� ����
	BoundingBox octant[8];
	octant[0] = BoundingBox((center - extents) / 2.0f, XMFLOAT3(half, half, half));
	octant[1] = BoundingBox((center + XMFLOAT3(-extents.x,-extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[2] = BoundingBox((center + XMFLOAT3(extents.x, -extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[3] = BoundingBox((center + XMFLOAT3(extents.x, -extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[5] = BoundingBox((center + extents) / 2.0f, XMFLOAT3(half, half, half));
	octant[4] = BoundingBox((center + XMFLOAT3(-extents.x, extents.y, extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[6] = BoundingBox((center + XMFLOAT3(extents.x, extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));
	octant[7] = BoundingBox((center + XMFLOAT3(-extents.x, extents.y, -extents.z)) / 2.0f, XMFLOAT3(half, half, half));

	// �� 8���� ����� ������Ʈ ����Ʈ
	std::list<std::shared_ptr<GameObject>> octList[8];

	// ���� ������ �ִ� ������Ʈ ����Ʈ�κ��� ���־� �� ������Ʈ���� ��� ����Ʈ
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

	// ���� ��忡 ���Ե� ������Ʈ�� �� ��忡������ �����Ѵ�.
	for (auto& obj : delist)
		mObjectList.remove(obj);

	// ���� ��忡 ������Ʈ�� �����Ѵٸ� BuildTree�� ��������� �����Ѵ�.
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

	// ���� �ٿ�� �ڽ��� 8���� �������� ����
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

	// �� ��尡 ������Ʈ�� ������ ���� �ʴٸ� ī��Ʈ �ٿ��� �����ϰ�,
	// ī��Ʈ �ٿ��� 0�� �� �ÿ� �� ��带 �����Ѵ�.
	// �ױ� ���� ��带 �ٽ� ����ϰ� �ƴٸ� ������ �� ��� �ø���.
	// �̷ν� ���ʿ��� �޸� �Ҵ� �� ������ ���� �� �ִ�.
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
		// �� �Լ��� Tick�� �Ҹ��� �� ���Ŀ� WorldUpdate�� Set�ǹǷ�
		// Update�Լ��� Object�� Tick�Լ� ������ �ҷ����� �Ѵ�.
		if (obj->GetIsMovable() && obj->GetIsWorldUpdate())
			movedObjects.emplace_front(obj);
	}

	mHasChildren = false;
	// CurrLife�� 0�� �� ���� �����Ѵ�.
	for (int flags = mActiveNodes, index = 0; flags > 0; flags >>= 1, ++index)
	{
		if (mChildNodes[index] != nullptr)
		{
			mHasChildren = true;

			if ((flags & 1) == 1 && (mChildNodes[index]->GetCurrentLife() == 0))
			{
				// ������Ʈ�� ������ ��, ��带 �����ؼ��� �ȵȴ�.
				if (mChildNodes[index]->GetObjectCount() > 0)
				{
					mChildNodes[index]->SetCurrentLife(-1);
				}
				// ������Ʈ�� ����, CurrLife = 0�̶�� ��带 �����Ѵ�.
				else
				{
					delete mChildNodes[index];
					mChildNodes[index] = nullptr;
					mActiveNodes ^= (byte)(1 << index);
				}
			}
		}
	}

	// �ڽ� ���鵵 ������Ʈ�Ѵ�.
	for (int flags = mActiveNodes, index = 0; flags > 0; flags >>= 1, index++)
	{
		if ((flags & 1) == 1)
		{
			if (mChildNodes != nullptr && mChildNodes[index] != nullptr)
				mChildNodes[index]->Update(deltaTime);
		}
	}

	// ������Ʈ�� �������ٸ� �θ� ���� �̵��Ͽ� �˸´� ��带 �ٽ� ã�´�.
	for (auto& obj : movedObjects)
	{
		Octree* currentNode = this;

		// ������Ʈ�� �ٿ�� �ڽ��� ������ ������ �θ� ��带 ����ؼ� ã�ư���.
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

		// ���� ��忡�� ������ ������Ʈ�� �����ϰ�,
		// �˸��� ��忡 �ٽ� �����Ѵ�.
		currentNode->Insert(obj);
		mObjectList.remove(obj);
	}

	// �θ��忡 �ִ� ������Ʈ��� 1:1�˻縦 �� ��
	// ���� ��忡 �ִ� ������Ʈ���� �浹�˻縦 �Ѵ�.
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