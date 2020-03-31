#pragma once

#include "Defines.h"
#include "Vector.h"
#include "d3dx12.h"
#include <DirectXCollision.h>
#include <vector>

struct LineVertex;

/*
프레임워크에서 디버깅하기 위한 클래스
디버그 용 라인, 메쉬 등을 그린다.
*/
class D3DDebug
{
public:
	struct DebugData
	{
		class Mesh* debugMesh;
		float time;
		// 수명이 다 될 시에 디버그 메쉬의 메모리를 해제해야 하는데
		// 이전 프레임에서 이 리소스를 사용하고 있으므로
		// NUM_FRAME_RESOURCES만큼 프레임이 지났을 때 메모리를 해제한다.
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

	// primitive topology에 따라 디버그 메쉬를 만든다.
	void BuildDebugMeshWithTopology(DebugData& data, const std::vector<LineVertex>& vertices, const float time, D3D_PRIMITIVE_TOPOLOGY topology);
	// primitive topology가 lint list인 디버그 메쉬를 만든다.
	void BuildDebugMesh(DebugData& data, const std::vector<LineVertex>& vertices, const float time);

	// 디버그 메쉬를 생성하는 명령어를 수행한다.
	void ExcuteBuild();

private:
	ID3D12Device* d3dDevice = nullptr;

	// 디버그 메쉬를 생성한 커맨드 오브젝트를 따로 둔다.
	// 프레임워크에 종속되지 않기 위함이다.
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> meshBuildCmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> meshBuildCmdAlloc;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 currentFence = 0;

	// true일 시, 디버그 메쉬를 만들기 위한 명령어를 수행한다.
	bool isMeshBuild = false;

	std::list<DebugData> debugDatas;
};