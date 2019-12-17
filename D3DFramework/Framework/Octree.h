#pragma once

#include "pch.h"

class Octree
{
public:
	Octree(const DirectX::BoundingBox& boundingBox, const std::list<std::shared_ptr<class GameObject>>& objList); 
	Octree(const DirectX::BoundingBox& boundingBox);

public:
	void BuildTree(); // ������ �ִ� ������Ʈ ����Ʈ�� ����Ʈ���� �����Ѵ�.
	bool Insert(std::shared_ptr<class GameObject> obj); // ����Ʈ���� ������ ��忡 ������Ʈ�� �����Ѵ�.

	// ������Ʈ�� �������� ��쿡 ���� ������Ʈ �Ѵ�. 
	// Update�Լ��� Object�� Tick�Լ� ������ �ҷ����� �Ѵ�.
	void Update(float deltaTime); 

	void DestroyObjects();

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
	void GetChildObjectList(std::list<std::shared_ptr<class GameObject>>& objList) const;

public:
	static inline bool mTreeReady = false;
	static inline bool mTreeBuilt = false;

	// �ٿ�� �ڽ��� �� �� �ִ� �ּ� ������
	const static inline float MIN_SIZE = 1.0f;

private:
	// �� ����Ʈ���� AABB���� ������ �ִٰ� �����Ѵ�.
	DirectX::BoundingBox mBoundingBox;

	// �� ��尡 ������ �ִ� ������Ʈ ����Ʈ
	std::list<std::shared_ptr<class GameObject>> mObjectList;

	Octree* mChildNodes[8];
	Octree* mParent = nullptr;

	// 8���� Child Node �� ���Ǵ� ���� ��带 ��Ʈ�� ǥ���Ѵ�.
	byte mActiveNodes = 0;

	UINT mObjectCount = 0;
	bool mHasChildren = false;

	// �� ��带 �����ϱ� ���� ��ٷ����� ������ ��
	// �޸� �Ҵ� �� ���Ŵ� ��� �����̹Ƿ� ���� ���ɸ��� ����
	// �ٷ� �������� �ʰ�, �������� ������ �ʴ� ���� �ð��� ������ �����Ѵ�.
	int mMaxLifespan = 8; 
	int mCurrLife = -1;
};