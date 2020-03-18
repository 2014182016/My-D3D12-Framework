#pragma once

#include "pch.h"


class D3DDebug
{
public:
	struct DebugData
	{
		class Mesh* mDebugMesh;
		float mTime;
		int mFrame = NUM_FRAME_RESOURCES;
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
	void DrawLine(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const float time = 3.0f,
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawRay(const DirectX::XMVECTOR& p1, const DirectX::XMVECTOR& p2, const float time = 3.0f,
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents, const float time = 3.0f,
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawSphere(const DirectX::XMFLOAT3& center, const float radius, const float time = 3.0f,
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawRing(const DirectX::XMVECTOR& center, const DirectX::XMVECTOR& majorAxis, const DirectX::XMVECTOR& minorAxis,
		const float time = 3.0f, const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	
	void Draw(const DirectX::BoundingFrustum& frustum, const float time = 3.0f, 
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void Draw(const DirectX::BoundingBox& aabb, const float time = 3.0f,
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void Draw(const DirectX::BoundingOrientedBox& obb, const float time = 3.0f,
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void Draw(const DirectX::BoundingSphere& sphere, const float time = 3.0f,
		const DirectX::XMFLOAT4& color = (DirectX::XMFLOAT4)DirectX::Colors::Red);

private:
	void Reset(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc);
	void ExcuteBuild();
	void FlushCommandQueue();
	void BuildDebugMeshWithTopology(DebugData& data, const std::vector<struct LineVertex>& vertices, const float time, D3D_PRIMITIVE_TOPOLOGY topology);
	void BuildDebugMesh(DebugData& data, const std::vector<struct LineVertex>& vertices, const float time);

public:
	static inline D3DDebug* instance = nullptr;

private:
	ID3D12Device* mDevice = nullptr;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mMeshBuildCmdList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mMeshBuildCmdAlloc;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCmdQueue;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	std::list<DebugData> mDebugDatas;
	bool mIsMeshBuild = false;
};