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
	// 오브젝트가 1개 이하일 경우 BuildTree를 종료한다.
	if (weakObjectList.size() <= 1)
		return;

	if (boundingBox.Extents.x <= MIN_SIZE)
		return;

	BoundingBox octant[OCT];
	// 현재 바운딩 박스를 8개의 공간으로 분할
	SpatialDivision(octant, boundingBox.Center, boundingBox.Extents);

	// 각 8개의 노드의 오브젝트 리스트
	std::list<std::shared_ptr<GameObject>> octList[8];
	// 현재 가지고 있는 오브젝트 리스트로부터 없애야 할 오브젝트들을 담는 리스트
	std::list<std::shared_ptr<GameObject>> delist;

	for (const auto& weakObj : weakObjectList)
	{
		auto obj = weakObj.lock();
		if (obj == nullptr)
			continue;

		for (int i = 0; i < OCT; ++i)
		{
			// 현재 노드의 바운딩 박스가 객체를 완전히 포함하고
			// 있다면 하위 노드에 포함시키도록 리스트에 추가한다.
			if (Physics::Contain(obj.get(), octant[i]))
			{
				octList[i].emplace_front(obj);
				delist.emplace_front(obj);
			}
		}
	}

	delist.unique();

	// 하위 노드에 포함된 오브젝트는 이 노드에서부터 삭제한다.
	for (auto& obj : delist)
	{
		DeleteObject(obj);
	}

	// 자식 노드에 오브젝트가 존재한다면 BuildTree를 재귀적으로 실행한다.
	for (int i = 0; i < OCT; ++i)
	{
		if (!octList[i].empty())
		{
			// 자식 노드를 생성하고 자식 노드도 같은 일을 반복한다.
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

	// 해당 노드에 저장된 객체가 없다면 공간을 나눌 필요가 없다.
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
	// 현재 바운딩 박스를 8개의 공간으로 분할
	SpatialDivision(octant, boundingBox.Center, boundingBox.Extents);

	for (int i = 0; i < OCT; ++i)
	{
		// 현재 바운딩 박스가 객체를 완전히 포함한다면
		if (Physics::Contain(obj.get(), octant[i]))
		{
			// 자식 노드가 존재한다면
			if (childNodes[i] != nullptr)
			{
				// 자식 노드에 객체를 삽입한다.
				return childNodes[i]->Insert(obj);
			}
			// 아니라면
			else
			{
				// 자식 노드를 생성하고 객체를 삽입한다.
				childNodes[i] = CreateNode(octant[i], obj);
				activeNodes |= (UINT8)(1 << i);
				return childNodes[i]->Insert(obj);
			}
		}
	}

	// 모든 자식 노드에 완전히 포함되지 않는다면
	// 현재 노드에 삽입한다.
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

	// 이 노드가 오브젝트를 가지고 있지 않다면 카운트 다운을 시작하고,
	// 카운트 다운이 0이 될 시에 이 노드를 삭제한다.
	// 죽기 전에 노드를 다시 사용하게 됐다면 수명을 두 배로 늘린다.
	// 이로써 불필요한 메모리 할당 및 해제를 피할 수 있다.
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

	// 움직인 노드들을 리스트로 따로 분류한다.
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
			// 이 함수는 Tick이 불리고 난 이후에 WorldUpdate가 설정되므로
			// Update함수는 Object의 Tick함수 이전에 불려져야 한다.
			if (obj->GetIsWorldUpdate())
				movedObjects.emplace_front(obj);
			++iter;
		}
	}

	hasChildren = false;
	// CurrLife가 0이 된 노드는 삭제한다.
	for (int flags = activeNodes, index = 0; flags > 0; flags >>= 1, ++index)
	{
		if (childNodes[index] != nullptr)
		{
			hasChildren = true;

			if ((flags & 1) == 1 && (childNodes[index]->currLife == 0))
			{
				// 오브젝트가 존재할 떄, 노드를 삭제해서는 안된다.
				if (childNodes[index]->GetObjectCount() > 0)
				{
					childNodes[index]->currLife = -1;
				}
				// 오브젝트가 없고, CurrLife = 0이라면 노드를 삭제한다.
				else
				{
					delete childNodes[index];
					childNodes[index] = nullptr;
					activeNodes ^= (UINT8)(1 << index);
				}
			}
		}
	}

	// 자식 노드들도 업데이트한다.
	for (int flags = activeNodes, index = 0; flags > 0; flags >>= 1, index++)
	{
		if ((flags & 1) == 1)
		{
			if (childNodes != nullptr && childNodes[index] != nullptr)
				childNodes[index]->Update(deltaTime);
		}
	}
	
	/*****************************************************************************/
	// 모든 자식 노드들이 업데이트 함수를 수행한 이후이다.

	// 오브젝트가 움직였다면 부모 노드로 이동하여 알맞는 노드를 다시 찾는다.
	for (auto& obj : movedObjects)
	{
		Octree* currentNode = this;
		auto objBounding = obj->GetCollisionBounding();

		// 오브젝트의 바운딩 박스를 완전히 포함할 부모 노드를 계속해서 찾아간다.
		while (Physics::Contain(obj.get(), currentNode->GetBoundingBox()) == false)
		{
			if (currentNode->parent != nullptr)
				currentNode = currentNode->parent;
			else
				break;
		}
	
		if (currentNode == this)
			continue;

		// 현재 노드에서 움직인 오브젝트를 삭제하고,
		// 알맞은 노드에 다시 삽입한다.
		currentNode->Insert(obj);
		DeleteObject(obj);
	}

	// 부모노드에 있는 오브젝트들과 1:1검사를 한 뒤
	// 현재 노드에 있는 오브젝트끼리 충돌검사를 한다.
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
			// 오브젝트끼리 충돌했다면
			if (Physics::IsCollision(obj.get(), other.get()))
			{
				// 충돌했을 때의 행동을 수행한다.
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
			// 오브젝트끼리 충돌했다면
			if (Physics::IsCollision(obj.get(), other.get()))
			{
				// 충돌했을 때의 행동을 수행한다.
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
	// UID를 확인하여 객체가 동일한지 확인 후 삭제한다.
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