#pragma once

#include "pch.h"
#include "BufferMemoryPool.hpp"
#include "Structures.h"
#include "Enums.h"

class D3DDebug
{
public:
	D3DDebug();
	~D3DDebug();
	D3DDebug& operator=(const D3DDebug& rhs) = delete;

public:
	void Reset(); // Render하고 난 이후에 다시 Reset을 해주어야 한다.
	void Update(ID3D12GraphicsCommandList* cmdList, BufferMemoryPool<DebugData>* currDebugPool, const int index);
	void RenderDebug(ID3D12GraphicsCommandList* cmdList, BufferMemoryPool<InstanceConstants>* currInstancePool) const;
	void RenderLine(ID3D12GraphicsCommandList* cmdList, float deltaTime);

	void AddDebugData(DebugData& data, const DebugType type);
	void AddDebugData(DebugData&& data, const DebugType type);
	void AddDebugData(DebugData&& data, const CollisionType type);
	void AddDebugData(std::vector<DebugData>& datas, const DebugType type);

	void DrawLine(DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawLine(DirectX::XMVECTOR p1, DirectX::XMVECTOR p2, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawCube(DirectX::XMFLOAT3 center, float width, float height, float depth, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawCube(DirectX::XMVECTOR center, float width, float height, float depth, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawCube(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extent, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawCube(DirectX::XMVECTOR center, DirectX::XMVECTOR extent, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawCube(DirectX::XMFLOAT3 center, float width, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawCube(DirectX::XMVECTOR center, float width, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawRing(DirectX::XMFLOAT3 center, DirectX::XMVECTOR majorAxis, DirectX::XMVECTOR minorAxis, 
		float radius, float time = 5.0f, 
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawSphere(DirectX::XMFLOAT3 center, float radius, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);
	void DrawSphere(DirectX::XMVECTOR center, float radius, float time = 5.0f,
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::Red);

	UINT GetAllDebugDataCount() const;

public:
	static D3DDebug* GetInstance() { return mInstance; }

private:
	void CreateDirectObjects();
	void FlushCommandQueue();
	void CommandListReset();
	void CommandListExcute();

private:
	static inline D3DDebug* mInstance = nullptr;

	std::vector<DebugData> mDebugDatas[(int)DebugType::Count];
	std::vector<std::unique_ptr<class MeshGeometry>> mDebugMeshes;
	// line은 임시적으로 그리는 객체이며, int형으로 가지는 ms단위의 시간이 지나면 삭제한다.
	std::forward_list<std::pair<float, std::unique_ptr<class MeshGeometry>>> mLines;

	UINT mInstanceCBByteSize = 0;
	UINT mDebugCBIndex = 0;

	// Debug Mesh들을 실행하기 위한 Direct Objects
	// Framework에서 CommandList를 해당 Allocator로 리셋한 후 CommandList를 사용해야 하지만
	// 그 이외에 상황에서 CommandList를 사용하려하면 Allocator에 명령이 저장되지 않는다.
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;
};