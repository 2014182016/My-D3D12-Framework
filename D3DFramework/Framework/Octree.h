#pragma once

#include "pch.h"

class Octree
{
public:
	Octree(const DirectX::BoundingBox& boundingBox, const std::list<std::shared_ptr<class GameObject>>& objList); // 사용할 공간의 크기와 게임 오브젝트를 받아 팔진트리를 생성한다.
	Octree(const std::list<std::shared_ptr<class GameObject>>& objList); // 게임 오브젝트의 딱 맞는 바운딩 박스를 생성한다.
	Octree(const DirectX::BoundingBox& boundingBox); // 게임 오브젝트 없이 바운딩 박스만을 생성한다.

public:
	void BuildTree(); // 가지고 있는 오브젝트 리스트로 팔진트리를 생성한다.
	bool Insert(std::shared_ptr<class GameObject> obj); // 팔진트리에 적당한 노드에 오브젝트를 삽입한다.

	// 오브젝트가 움직였을 경우에 대해 업데이트 한다. 
	// Update함수는 Object의 Tick함수 이후에 불려져야 한다.
	void Update(float deltaTime); 

public:
	inline Octree* GetParent() const { return mParent; }
	inline void SetParent(Octree* parent) { mParent = parent; }

	inline void SetCurrentLife(int value) { mCurrLife = value; }
	inline int GetCurrentLife() const { return mCurrLife; }

	inline std::list<std::shared_ptr<class GameObject>> GetObjectList() const { return mObjectList; }
	inline UINT GetObjectCount() const { return mObjectCount; }

	inline DirectX::BoundingBox GetBoundingBox() const { return mBoundingBox; }
	void GetBoundingWorlds(std::vector<DirectX::XMFLOAT4X4>& worlds) const;

	void GetIntersectObjects(const DirectX::BoundingBox& bounding, std::list<std::shared_ptr<class GameObject>>& objects);

private:
	Octree* CreateNode(const DirectX::BoundingBox& boundingBox, std::list<std::shared_ptr<class GameObject>> objList);
	Octree* CreateNode(const DirectX::BoundingBox& boundingBox, std::shared_ptr<class GameObject> obj);
	void GetParentObjectList(std::list<std::shared_ptr<class GameObject>>& objList) const;

public:
	static inline bool mTreeReady = false;
	static inline bool mTreeBuilt = false;

	// 바운딩 박스에 들어갈 수 있는 최소 사이즈
	const static inline float MIN_SIZE = 1.0f;

private:
	// 이 팔진트리는 AABB만을 가지고 있다고 가정한다.
	DirectX::BoundingBox mBoundingBox;

	// 이 노드가 가지고 있는 오브젝트 리스트
	std::list<std::shared_ptr<class GameObject>> mObjectList;

	Octree* mChildNodes[8];
	Octree* mParent = nullptr;

	// 8개의 Child Node 중 사용되는 중인 노드를 비트로 표현한다.
	byte mActiveNodes = 0;

	UINT mObjectCount = 0;
	bool mHasChildren = false;

	// 이 노드를 제거하기 전에 기다려야할 프레임 수
	// 메모리 할당 및 제거는 비싼 연산이므로 자주 사용될만한 노드는
	// 바로 삭제하지 않고, 오랫동안 사용되지 않는 노드는 시간이 지나면 삭제한다.
	int mMaxLifespan = 8; 
	int mCurrLife = -1;
};