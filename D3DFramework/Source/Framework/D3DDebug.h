#pragma once

#include "Defines.h"
#include "Vector.h"
#include "d3dx12.h"
#include <DirectXCollision.h>
#include <vector>

struct LineVertex;

/*
�����ӿ�ũ���� ������ϱ� ���� Ŭ����
����� �� ����, �޽� ���� �׸���.
*/
class D3DDebug
{
public:
	struct DebugData
	{
		class Mesh* debugMesh;
		float time;
		// ������ �� �� �ÿ� ����� �޽��� �޸𸮸� �����ؾ� �ϴµ�
		// ���� �����ӿ��� �� ���ҽ��� ����ϰ� �����Ƿ�
		// NUM_FRAME_RESOURCES��ŭ �������� ������ �� �޸𸮸� �����Ѵ�.
		INT32 frame = NUM_FRAME_RESOURCES;
	};

public:
	D3DDebug();
	D3DDebug& operator=(const D3DDebug& rhs) = delete;
	~D3DDebug();

public:
	static D3DDebug* GetInstance();

public:
	void CreateCommandObjects(ID3D12Device* device);
	void Update(float deltaTime);
	void Render(ID3D12GraphicsCommandList* cmdList);
	void Clear();

public:
	void DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2, const float time = 3.0f,
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	void DrawRay(const XMVECTOR& p1, const XMVECTOR& p2, const float time = 3.0f,
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	void DrawBox(const XMFLOAT3& center, const XMFLOAT3& extents, const float time = 3.0f,
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	void DrawSphere(const XMFLOAT3& center, const float radius, const float time = 3.0f,
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	void DrawRing(const XMVECTOR& center, const XMVECTOR& majorAxis, const XMVECTOR& minorAxis,
		const float time = 3.0f, const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	
	void Draw(const BoundingFrustum& frustum, const float time = 3.0f, 
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	void Draw(const BoundingBox& aabb, const float time = 3.0f,
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	void Draw(const BoundingOrientedBox& obb, const float time = 3.0f,
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);
	void Draw(const BoundingSphere& sphere, const float time = 3.0f,
		const XMFLOAT4& color = (XMFLOAT4)Colors::Red);

private:
	void Reset(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc);
	void FlushCommandQueue();

	// primitive topology�� ���� ����� �޽��� �����.
	void BuildDebugMeshWithTopology(DebugData& data, const std::vector<LineVertex>& vertices, const float time, D3D_PRIMITIVE_TOPOLOGY topology);
	// primitive topology�� lint list�� ����� �޽��� �����.
	void BuildDebugMesh(DebugData& data, const std::vector<LineVertex>& vertices, const float time);

	// ����� �޽��� �����ϴ� ��ɾ �����Ѵ�.
	void ExcuteBuild();

private:
	ID3D12Device* d3dDevice = nullptr;

	// ����� �޽��� ������ Ŀ�ǵ� ������Ʈ�� ���� �д�.
	// �����ӿ�ũ�� ���ӵ��� �ʱ� �����̴�.
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> meshBuildCmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> meshBuildCmdAlloc;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 currentFence = 0;

	// true�� ��, ����� �޽��� ����� ���� ��ɾ �����Ѵ�.
	bool isMeshBuild = false;

	std::list<DebugData> debugDatas;
};