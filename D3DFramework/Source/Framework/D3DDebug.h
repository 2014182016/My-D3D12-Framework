#pragma once

#include "pch.h"
#include "BufferMemoryPool.hpp"
#include "Enums.h"

class D3DDebug
{
public:
	D3DDebug();
	~D3DDebug();
	D3DDebug& operator=(const D3DDebug& rhs) = delete;

public:
	void Initialize(ID3D12Device* device);

	void Reset(); // Render�ϰ� �� ���Ŀ� �ٽ� Reset�� ���־�� �Ѵ�.
	void RenderLine(ID3D12GraphicsCommandList* cmdList, float deltaTime);

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

	UINT GetDebugDataCount() const;

public:
	static D3DDebug* GetInstance();

	inline std::vector<struct DebugConstants>& GetDebugConstants(int index) { return mDebugConstants[index]; }
	inline class MeshGeometry* GetDebugMesh(int index) const { return mDebugMeshes[index].get(); }

private:
	void CreateDirectObjects();
	void FlushCommandQueue();
	void CommandListReset();
	void CommandListExcute();
	void CreateDebugMeshes();

private:
	static inline D3DDebug* instance = nullptr;
	static inline bool isInitialized = false;

	std::vector<DebugConstants> mDebugConstants[(int)DebugType::Count];
	std::array<std::unique_ptr<class MeshGeometry>, (int)DebugType::Count> mDebugMeshes;
	// line�� �ӽ������� �׸��� ��ü�̸�, int������ ������ ms������ �ð��� ������ �����Ѵ�.
	std::forward_list<std::pair<float, std::unique_ptr<class MeshGeometry>>> mLines;

	// Debug Mesh���� �����ϱ� ���� Direct Objects
	// Framework���� CommandList�� �ش� Allocator�� ������ �� CommandList�� ����ؾ� ������
	// �� �̿ܿ� ��Ȳ���� CommandList�� ����Ϸ��ϸ� Allocator�� ����� ������� �ʴ´�.
	ID3D12Device* md3dDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;
};