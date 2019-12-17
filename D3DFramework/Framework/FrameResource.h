#pragma once

#include "pch.h"
#include "Structures.h"
#include "Enums.h"
#include "BufferMemoryPool.hpp"

// CPU가 한 프레임의 명령 목록들을 구축하는 데 필요한 자원들을
// 대표하는 구조체
struct FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT lightCount, UINT materialCount)
	{
		ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(mCmdListAlloc.GetAddressOf())));

		mPassPool = std::make_unique<BufferMemoryPool<PassConstants>>(device, passCount, true);
		mObjectPool = std::make_unique<BufferMemoryPool<ObjectConstants>>(device, objectCount, true);
		mLightBufferPool = std::make_unique<BufferMemoryPool<LightData>>(device, lightCount, false);
		mMaterialBufferPool = std::make_unique<BufferMemoryPool<MaterialData>>(device, materialCount, false);
		mDebugPool = std::make_unique<BufferMemoryPool<DebugData>>(device, objectCount, false);
		mInstancePool = std::make_unique<BufferMemoryPool<InstanceConstants>>(device, (int)DebugType::Count, true);
	}
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource() { }
 
public:
	// 명령 할당자는 GPU가 명령들을 다 처리한 후 재설정해야한다.
	// 따라서 프레임마다 할당자가 필요하다.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAlloc;

	// 상수 버퍼는 그것을 참조하는 명령들을 GPU가 다 처리한 후에 갱신해야 한다.
	// 따라서 프레임마다 상수 버퍼를 새로 만들어야 한다.
	std::unique_ptr<BufferMemoryPool<PassConstants>> mPassPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ObjectConstants>> mObjectPool = nullptr;
	std::unique_ptr<BufferMemoryPool<LightData>> mLightBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<MaterialData>> mMaterialBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<DebugData>> mDebugPool = nullptr;
	std::unique_ptr<BufferMemoryPool<InstanceConstants>> mInstancePool = nullptr;

	// Fence는 현재 울타리 지점까지의 명령들을 표시하는 값이다.
	// 이 값은 GPU가 아직 이 프레임 자원들을 사용하고 있는지 판정하는 용도로 쓰인다.
	UINT64 mFence = 0;
};