#pragma once

#include "pch.h"
#include "Structures.h"

template<typename T>
class UploadBuffer;


// CPU가 한 프레임의 명령 목록들을 구축하는 데 필요한 자원들을
// 대표하는 클래스
class FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	// 명령 할당자는 GPU가 명령들을 다 처리한 후 재설정해야한다.
	// 따라서 프레임마다 할당자가 필요하다.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAlloc;

	// 상수 버퍼는 그것을 참조하는 명령들을 GPU가 다 처리한 후에 갱신해야 한다.
	// 따라서 프레임마다 상수 버퍼를 새로 만들어야 한다.
	std::unique_ptr<UploadBuffer<PassConstants>> mPassCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<MaterialData>> mMaterialBuffer = nullptr;

	// Fence는 현재 울타리 지점까지의 명령들을 표시하는 값이다.
	// 이 값은 GPU가 아직 이 프레임 자원들을 사용하고 있는지 판정하는 용도로 쓰인다.
	UINT64 mFence = 0;
};