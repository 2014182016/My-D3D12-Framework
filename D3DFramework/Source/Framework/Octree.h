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
�����ӿ�ũ���� ���Ǵ� ��� ���� ������Ʈ����
�浹�� ����Ѵ�. �ϳ��� ū �ٿ�� �ڽ� ���ο� ��ü��
�����Ѵٴ� �����Ͽ� ��ü�� ��ġ�� ���� ������ �����Ͽ�
�浹 �˰����� ����ȭ�Ѵ�.
*/
class Octree
{
public:
	Octree(const DirectX::BoundingBox& boundingBox, const std::list<std::shared_ptr<GameObject>>& objList); 
	Octree(const DirectX::BoundingBox& boundingBox);

public:
	// ������ �ִ� ������Ʈ ����Ʈ�� ����Ʈ���� �����Ѵ�.
	void BuildTree();
	// ����Ʈ���� ������ ��忡 ������Ʈ�� �����Ѵ�.
	bool Insert(std::shared_ptr<GameObject> obj);

	// ������Ʈ�� �������� ��쿡 ���� ������Ʈ �Ѵ�. 
	// Update�Լ��� Object�� Tick�Լ� ������ �ҷ����� �Ѵ�.
	void Update(float deltaTime); 

	// ���� ��尡 ������ �ִ� �ٿ�� �ڽ��� ���� ����� ��ȯ�Ѵ�.
	void GetBoundingWorlds(std::vector<DirectX::XMFLOAT4X4>& worlds) const;
	// D3DDebug�� ����Ͽ� �ٿ�� �ڽ��� �׸���.
	void DrawDebug();

	UINT32 GetObjectCount() const;
	DirectX::BoundingBox GetBoundingBox() const;

private:
	// ���ο� ��带 �����Ѵ�.
	Octree* CreateNode(const DirectX::BoundingBox& boundingBox, std::list<std::shared_ptr<GameObject>> objList);
	Octree* CreateNode(const DirectX::BoundingBox& boundingBox, std::shared_ptr<GameObject> obj);

	// �ٿ�� �ڽ��� �̷���� ������ 8���� ������.
	void SpatialDivision(DirectX::BoundingBox* octant, const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents) const;
 
	// �θ� ���κ��� ��ü ����Ʈ�� �����Ѵ�.
	void GetParentObjectList(std::list<std::weak_ptr<GameObject>>& objList) const;
	// �ڽ� ���κ��� ��ü ����Ʈ�� �����Ѵ�.
	void GetChildObjectList(std::list<std::weak_ptr<GameObject>>& objList) const;
	// ��ü ����Ʈ���� �ش� ��ü�� �����Ѵ�.
	void DeleteObject(std::shared_ptr<GameObject> obj);

	// �ش� ��ü�� �浹 �������� ���θ� Ȯ���Ѵ�.
	bool IsEnabledCollision(std::shared_ptr<GameObject> obj);

public:
	static inline bool treeReady = false;
	static inline bool treeBuilt = false;

public:
	Octree* parent = nullptr;

	// �� ��尡 ������ �ִ� ������Ʈ ����Ʈ
	// shared_ptr�� ���� ������ ������ �ִ� weak_ptr�̹Ƿ�
	// ����� ���� lock()�Լ��� �̿��Ͽ� shared_ptr�� ��ȯ�Ѵ�.
	std::list<std::weak_ptr<GameObject>> weakObjectList;

	// �� ��带 �����ϱ� ���� ��ٷ����� ������ ��
	// �޸� �Ҵ� �� ���Ŵ� ��� �����̹Ƿ� ���� ���ɸ��� ����
	// �ٷ� �������� �ʰ�, �������� ������ �ʴ� ���� �ð��� ������ �����Ѵ�.
	INT32 maxLifespan = 8;
	INT32 currLife = -1;

private:
	// �� ����Ʈ���� AABB���� ������ �ִٰ� �����Ѵ�.
	DirectX::BoundingBox boundingBox;

	// �ش� ��忡�� �п��� �ڽĳ��
	Octree* childNodes[8];

	// 8���� Child Node �� ���Ǵ� ���� ��带 ��Ʈ�� ǥ���Ѵ�.
	UINT8 activeNodes = 0;

	// �ڽ� ��带 �ϳ��� ������ �ִٸ� true�̴�.
	bool hasChildren = false;
};