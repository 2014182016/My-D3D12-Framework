#pragma once

#include <DirectXCollision.h>
#include <list>
#include <memory>
#include <basetsd.h>
#include <vector>

class GameObject;

#define OCT 8
#define MIN_SIZE 1.0f

/*
프레임워크에서 사용되는 모든 게임 오브젝트들의
충돌을 담당한다. 하나의 큰 바운딩 박스 내부에 객체가
존재한다는 가정하에 객체의 위치에 따라 공간을 분할하여
충돌 알고리즘을 최적화한다.
*/
class Octree
{
public:
	Octree(const DirectX::BoundingBox& boundingBox, const std::list<std::shared_ptr<GameObject>>& objList); 
	Octree(const DirectX::BoundingBox& boundingBox);

public:
	// 가지고 있는 오브젝트 리스트로 팔진트리를 생성한다.
	void BuildTree();
	// 팔진트리에 적당한 노드에 오브젝트를 삽입한다.
	bool Insert(std::shared_ptr<GameObject> obj);

	// 오브젝트가 움직였을 경우에 대해 업데이트 한다. 
	// Update함수는 Object의 Tick함수 이전에 불려져야 한다.
	void Update(float deltaTime); 

	// 현재 노드가 가지고 있는 바운딩 박스의 월드 행렬을 반환한다.
	void GetBoundingWorlds(std::vector<DirectX::XMFLOAT4X4>& worlds) const;
	// D3DDebug를 사용하여 바운딩 박스를 그린다.
	void DrawDebug();

	UINT32 GetObjectCount() const;
	DirectX::BoundingBox GetBoundingBox() const;

private:
	// 새로운 노드를 생성한다.
	Octree* CreateNode(const DirectX::BoundingBox& boundingBox, std::list<std::shared_ptr<GameObject>> objList);
	Octree* CreateNode(const DirectX::BoundingBox& boundingBox, std::shared_ptr<GameObject> obj);

	// 바운딩 박스로 이루어진 공간을 8개로 나눈다.
	void SpatialDivision(DirectX::BoundingBox* octant, const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents) const;
 
	// 부모 노드로부터 객체 리스트를 복사한다.
	void GetParentObjectList(std::list<std::weak_ptr<GameObject>>& objList) const;
	// 자식 노드로부터 객체 리스트를 복사한다.
	void GetChildObjectList(std::list<std::weak_ptr<GameObject>>& objList) const;
	// 객체 리스트에서 해당 객체를 삭제한다.
	void DeleteObject(std::shared_ptr<GameObject> obj);

	// 해당 객체가 충돌 가능한지 여부를 확인한다.
	bool IsEnabledCollision(std::shared_ptr<GameObject> obj);

public:
	static inline bool treeReady = false;
	static inline bool treeBuilt = false;

public:
	Octree* parent = nullptr;

	// 이 노드가 가지고 있는 오브젝트 리스트
	// shared_ptr에 대한 참조만 가지고 있는 weak_ptr이므로
	// 사용할 때는 lock()함수를 이용하여 shared_ptr로 변환한다.
	std::list<std::weak_ptr<GameObject>> weakObjectList;

	// 이 노드를 제거하기 전에 기다려야할 프레임 수
	// 메모리 할당 및 제거는 비싼 연산이므로 자주 사용될만한 노드는
	// 바로 삭제하지 않고, 오랫동안 사용되지 않는 노드는 시간이 지나면 삭제한다.
	INT32 maxLifespan = 8;
	INT32 currLife = -1;

private:
	// 이 팔진트리는 AABB만을 가지고 있다고 가정한다.
	DirectX::BoundingBox boundingBox;

	// 해당 노드에서 분열된 자식노드
	Octree* childNodes[8];

	// 8개의 Child Node 중 사용되는 중인 노드를 비트로 표현한다.
	UINT8 activeNodes = 0;

	// 자식 노드를 하나라도 가지고 있다면 true이다.
	bool hasChildren = false;
};