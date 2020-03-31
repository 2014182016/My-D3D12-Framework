#include "../PrecompiledHeader/pch.h"
#include "Octree.h"
#include "Enumeration.h"
#include "D3DDebug.h"
#include "Physics.h"
#include "../Component/Mesh.h"
#include "../Object/GameObject.h"

inline XMFLOAT3 operator+(const XMFLOAT3& lhs, const XMFLOAT3& rhs)
{
	XMFLOAT3 result;
	
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	result.z = lhs.z + rhs.z;

	return result;
}

inline XMFLOAT3 operator-(const XMFLOAT3& lhs, const XMFLOAT3& rhs)
{
	XMFLOAT3 result;

	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	result.z = lhs.z - rhs.z;

	return result;
}

inline XMFLOAT3 operator/(const XMFLOAT3& lhs, const float scalar)
{
	XMFLOAT3 result;

	result.x = lhs.x / scalar;
	result.y = lhs.y / scalar;
	result.z = lhs.z / scalar;

	return result;
}

Octree::Octree(const BoundingBox& boundingBox, const std::list<std::shared_ptr<GameObject>>& objList)
{
	this->boundingBox = boundingBox;

	for (const auto& obj : objList)
	{
		if(IsEnabledCollision(obj))
			weakObjectList.emplace_front(obj);
	}

	for (int i = 0; i < OCT; ++i)
		childNodes[i] = nullptr;
}

Octree::Octree(const BoundingBox& boundingBox)
{
	this->boundingBox = boundingBox;

	for (int i = 0; i < 8; ++i)
		childNodes[i] = nullptr;
}

void Octree::BuildTree()
{
	// ������Ʈ�� 1�� ������ ��� BuildTree�� �����Ѵ�.
	if (weakObjectList.size() <= 1)
		return;

	if (boundingBox.Extents.x <= MIN_SIZE)
		return;

	BoundingBox octant[OCT];
	// ���� �ٿ�� �ڽ��� 8���� �������� ����
	SpatialDivision(octant, boundingBox.Center, boundingBox.Extents);

	// �� 8���� ����� ������Ʈ ����Ʈ
	std::list<std::shared_ptr<GameObject>> octList[8];
	// ���� ������ �ִ� ������Ʈ ����Ʈ�κ��� ���־� �� ������Ʈ���� ��� ����Ʈ
	std::list<std::shared_ptr<GameObject>> delist;

	for (const auto& weakObj : weakObjectList)
	{
		auto obj = weakObj.lock();
		if (obj == nullptr)
			continue;

		for (int i = 0; i < OCT; ++i)
		{
			// ���� ����� �ٿ�� �ڽ��� ��ü�� ������ �����ϰ�
			// �ִٸ� ���� ��忡 ���Խ�Ű���� ����Ʈ�� �߰��Ѵ�.
			if (Physics::Contain(obj.get(), octant[i]))
			{
				octList[i].emplace_front(obj);
				delist.emplace_front(obj);
			}
		}
	}

	delist.unique();

	// ���� ��忡 ���Ե� ������Ʈ�� �� ��忡������ �����Ѵ�.
	for (auto& obj : delist)
	{
		DeleteObject(obj);
	}

	// �ڽ� ��忡 ������Ʈ�� �����Ѵٸ� BuildTree�� ��������� �����Ѵ�.
	for (int i = 0; i < OCT; ++i)
	{
		if (!octList[i].empty())
		{
			// �ڽ� ��带 �����ϰ� �ڽ� ��嵵 ���� ���� �ݺ��Ѵ�.
			childNodes[i] = CreateNode(octant[i], octList[i]);
			activeNodes |= (UINT8)(1 << i);
			childNodes[i]->BuildTree();
		}
	}

	treeBuilt = true;
	treeReady = true;
}

bool Octree::Insert(std::shared_ptr<GameObject> obj)
{
	if (IsEnabledCollision(obj))
		return false;

	// �ش� ��忡 ����� ��ü�� ���ٸ� ������ ���� �ʿ䰡 ����.
	if (weakObjectList.size() == 0)
	{
		weakObjectList.emplace_front(obj);
		return true;
	}

	if (boundingBox.Extents.x <= MIN_SIZE)
	{
		weakObjectList.emplace_front(obj);
		return true;
	}

	BoundingBox octant[OCT];
	// ���� �ٿ�� �ڽ��� 8���� �������� ����
	SpatialDivision(octant, boundingBox.Center, boundingBox.Extents);

	for (int i = 0; i < OCT; ++i)
	{
		// ���� �ٿ�� �ڽ��� ��ü�� ������ �����Ѵٸ�
		if (Physics::Contain(obj.get(), octant[i]))
		{
			// �ڽ� ��尡 �����Ѵٸ�
			if (childNodes[i] != nullptr)
			{
				// �ڽ� ��忡 ��ü�� �����Ѵ�.
				return childNodes[i]->Insert(obj);
			}
			// �ƴ϶��
			else
			{
				// �ڽ� ��带 �����ϰ� ��ü�� �����Ѵ�.
				childNodes[i] = CreateNode(octant[i], obj);
				activeNodes |= (UINT8)(1 << i);
				return childNodes[i]->Insert(obj);
			}
		}
	}

	// ��� �ڽ� ��忡 ������ ���Ե��� �ʴ´ٸ�
	// ���� ��忡 �����Ѵ�.
	weakObjectList.emplace_front(obj);
	return true;
}

void Octree::Update(float deltaTime)
{
	if (treeBuilt == false || treeReady == false)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Octree isn't built or ready" << std::endl;
#endif
		return;
	}

	// �� ��尡 ������Ʈ�� ������ ���� �ʴٸ� ī��Ʈ �ٿ��� �����ϰ�,
	// ī��Ʈ �ٿ��� 0�� �� �ÿ� �� ��带 �����Ѵ�.
	// �ױ� ���� ��带 �ٽ� ����ϰ� �ƴٸ� ������ �� ��� �ø���.
	// �̷ν� ���ʿ��� �޸� �Ҵ� �� ������ ���� �� �ִ�.
	if (weakObjectList.size() == 0)
	{
		if (!hasChildren)
		{
			if (currLife == -1)
				currLife = maxLifespan;
			else if (currLife > 0)
				--currLife;
		}
	}
	else
	{
		if (currLife != -1)
		{
			if (maxLifespan <= 64)
				maxLifespan *= 2;
			currLife = -1;
		}
	}

	// ������ ������ ����Ʈ�� ���� �з��Ѵ�.
	std::list<std::shared_ptr<GameObject>> movedObjects;
	for (auto iter = weakObjectList.begin(); iter != weakObjectList.end();)
	{
		auto obj = iter->lock();
		if (obj == nullptr)
		{
			iter = weakObjectList.erase(iter);
		}
		else
		{
			// �� �Լ��� Tick�� �Ҹ��� �� ���Ŀ� WorldUpdate�� �����ǹǷ�
			// Update�Լ��� Object�� Tick�Լ� ������ �ҷ����� �Ѵ�.
			if (obj->GetIsWorldUpdate())
				movedObjects.emplace_front(obj);
			++iter;
		}
	}

	hasChildren = false;
	// CurrLife�� 0�� �� ���� �����Ѵ�.
	for (int flags = activeNodes, index = 0; flags > 0; flags >>= 1, ++index)
	{
		if (childNodes[index] != nullptr)
		{
			hasChildren = true;

			if ((flags & 1) == 1 && (childNodes[index]->currLife == 0))
			{
				// ������Ʈ�� ������ ��, ��带 �����ؼ��� �ȵȴ�.
				if (childNodes[index]->GetObjectCount() > 0)
				{
					childNodes[index]->currLife = -1;
				}
				// ������Ʈ�� ����, CurrLife = 0�̶�� ��带 �����Ѵ�.
				else
				{
					delete childNodes[index];
					childNodes[index] = nullptr;
					activeNodes ^= (UINT8)(1 << index);
				}
			}
		}
	}

	// �ڽ� ���鵵 ������Ʈ�Ѵ�.
	for (int flags = activeNodes, index = 0; flags > 0; flags >>= 1, index++)
	{
		if ((flags & 1) == 1)
		{
			if (childNodes != nullptr && childNodes[index] != nullptr)
				childNodes[index]->Update(deltaTime);
		}
	}
	
	/*****************************************************************************/
	// ��� �ڽ� ������ ������Ʈ �Լ��� ������ �����̴�.

	// ������Ʈ�� �������ٸ� �θ� ���� �̵��Ͽ� �˸´� ��带 �ٽ� ã�´�.
	for (auto& obj : movedObjects)
	{
		Octree* currentNode = this;
		auto objBounding = obj->GetCollisionBounding();

		// ������Ʈ�� �ٿ�� �ڽ��� ������ ������ �θ� ��带 ����ؼ� ã�ư���.
		while (Physics::Contain(obj.get(), currentNode->GetBoundingBox()) == false)
		{
			if (currentNode->parent != nullptr)
				currentNode = currentNode->parent;
			else
				break;
		}
	
		if (currentNode == this)
			continue;

		// ���� ��忡�� ������ ������Ʈ�� �����ϰ�,
		// �˸��� ��忡 �ٽ� �����Ѵ�.
		currentNode->Insert(obj);
		DeleteObject(obj);
	}

	// �θ��忡 �ִ� ������Ʈ��� 1:1�˻縦 �� ��
	// ���� ��忡 �ִ� ������Ʈ���� �浹�˻縦 �Ѵ�.
	std::list<std::weak_ptr<GameObject>> currentObjList, parentObjList;
	currentObjList = weakObjectList;
	GetParentObjectList(parentObjList);

	while (!parentObjList.empty())
	{
		auto obj = parentObjList.front().lock();
		parentObjList.pop_front();

		for (auto& weakOther : currentObjList)
		{
			auto other = weakOther.lock();
			// ������Ʈ���� �浹�ߴٸ�
			if (Physics::IsCollision(obj.get(), other.get()))
			{
				// �浹���� ���� �ൿ�� �����Ѵ�.
				Physics::Collide(obj.get(), other.get(), deltaTime);
			}
		}
	}

	while (!currentObjList.empty())
	{
		auto obj = currentObjList.front().lock();
		currentObjList.pop_front();

		for (auto& weakOther : currentObjList)
		{
			auto other = weakOther.lock();
			// ������Ʈ���� �浹�ߴٸ�
			if (Physics::IsCollision(obj.get(), other.get()))
			{
				// �浹���� ���� �ൿ�� �����Ѵ�.
				Physics::Collide(obj.get(), other.get(), deltaTime);
			}
		}
	}
}


Octree* Octree::CreateNode(const BoundingBox& boundingBox, std::list<std::shared_ptr<GameObject>> objList)
{
	if (objList.empty())
		return nullptr;

	Octree* newOctant = new Octree(boundingBox, objList);
	newOctant->parent = this;
	return newOctant;
}

Octree* Octree::CreateNode(const BoundingBox& boundingBox, std::shared_ptr<GameObject> obj)
{
	std::list<std::shared_ptr<GameObject>> objList;
	objList.emplace_front(obj);

	Octree* newOctant = new Octree(boundingBox, objList);
	newOctant->parent = this;
	return newOctant;
}

void Octree::SpatialDivision(BoundingBox* octant, const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents) const
{
	float half = extents.x / 2.0f;
	XMFLOAT3 halfExtents = { half, half, half };

	octant[0] = BoundingBox((center - extents) / 2.0f, halfExtents);
	octant[1] = BoundingBox((center + XMFLOAT3(-extents.x, -extents.y, extents.z)) / 2.0f, halfExtents);
	octant[2] = BoundingBox((center + XMFLOAT3(extents.x, -extents.y, extents.z)) / 2.0f, halfExtents);
	octant[3] = BoundingBox((center + XMFLOAT3(extents.x, -extents.y, -extents.z)) / 2.0f, halfExtents);
	octant[5] = BoundingBox((center + extents) / 2.0f, halfExtents);
	octant[4] = BoundingBox((center + XMFLOAT3(-extents.x, extents.y, extents.z)) / 2.0f, halfExtents);
	octant[6] = BoundingBox((center + XMFLOAT3(extents.x, extents.y, -extents.z)) / 2.0f, halfExtents);
	octant[7] = BoundingBox((center + XMFLOAT3(-extents.x, extents.y, -extents.z)) / 2.0f, halfExtents);
}

void Octree::GetBoundingWorlds(std::vector<XMFLOAT4X4>& worlds) const
{
	XMMATRIX translation = XMMatrixTranslation(boundingBox.Center.x, boundingBox.Center.y, boundingBox.Center.z);
	XMMATRIX scailing = XMMatrixScaling(boundingBox.Extents.x, boundingBox.Extents.y, boundingBox.Extents.z);
	XMMATRIX world = XMMatrixMultiply(scailing, translation);
	XMFLOAT4X4 world4x4f;
	XMStoreFloat4x4(&world4x4f, XMMatrixTranspose(world));
	worlds.push_back(std::move(world4x4f));

	for (int i = 0; i < OCT; ++i)
	{
		if (childNodes[i] != nullptr)
			childNodes[i]->GetBoundingWorlds(worlds);
	}
}

void Octree::GetParentObjectList(std::list<std::weak_ptr<GameObject>>& objList) const
{
	Octree* parentNode = parent;
	while (parentNode != nullptr)
	{
		const std::list<std::weak_ptr<GameObject>>& parentObjList = parentNode->weakObjectList;
		objList.insert(objList.end(), parentObjList.begin(), parentObjList.end());
		parentNode = parentNode->parent;
	}
}

void Octree::GetChildObjectList(std::list<std::weak_ptr<class GameObject>>& objList) const
{
	if (childNodes == nullptr)
		return;

	for (int i = 0; i < OCT; ++i)
	{
		if (childNodes[i] != nullptr)
		{
			const std::list<std::weak_ptr<class GameObject>> chileObjList = childNodes[i]->weakObjectList;
			objList.insert(objList.end(), chileObjList.begin(), chileObjList.end());
			childNodes[i]->GetChildObjectList(objList);
		}
	}
}

void Octree::DeleteObject(std::shared_ptr<class GameObject> obj)
{
	// UID�� Ȯ���Ͽ� ��ü�� �������� Ȯ�� �� �����Ѵ�.
	weakObjectList.remove_if([&obj](std::weak_ptr<GameObject>& weakObj)->bool
	{ auto listedObj = weakObj.lock();
	if (obj->GetUID() == listedObj->GetUID()) return true; return false; });
}

bool Octree::IsEnabledCollision(std::shared_ptr<GameObject> obj)
{
	CollisionType collisionType = obj->GetCollisionType();
	if (collisionType == CollisionType::None || collisionType == CollisionType::Point)
		return false;
	return true;
}

void Octree::DrawDebug()
{
	D3DDebug::GetInstance()->Draw(boundingBox, FLT_MAX, (XMFLOAT4)Colors::Green);

	for (int i = 0; i < OCT; ++i)
	{
		if (childNodes[i] != nullptr)
		{
			childNodes[i]->DrawDebug();
		}
	}
}

UINT32 Octree::GetObjectCount() const
{
	return (UINT32)weakObjectList.size();
}

BoundingBox Octree::GetBoundingBox() const
{
	return boundingBox;
}