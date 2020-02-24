#pragma once

#include "pch.h"
#include "Structures.h"
#include "Enums.h"
#include "BufferMemoryPool.hpp"

#define FRAME_PHASE 5

// CPU가 한 프레임의 명령 목록들을 구축하는 데 필요한 자원들을
// 대표하는 구조체
struct FrameResource
{
public:
	FrameResource(ID3D12Device* device, bool singleThread,
		UINT passCount, UINT objectCount, UINT lightCount, UINT materialCount,
		UINT widgetCount, UINT particleCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource() { }

public:
	D3D12_GPU_VIRTUAL_ADDRESS GetPassVirtualAddress() const {
		if (mPassPool->GetBuffer()) return mPassPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return 0; }
	D3D12_GPU_VIRTUAL_ADDRESS GetObjectVirtualAddress() const { 
		if (mObjectPool->GetBuffer()) return mObjectPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return 0; }
	D3D12_GPU_VIRTUAL_ADDRESS GetLightVirtualAddress() const { 
		if (mLightBufferPool->GetBuffer()) return mLightBufferPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return 0; }
	D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVirtualAddress() const {
		if (mMaterialBufferPool->GetBuffer()) return mMaterialBufferPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return 0; }
	D3D12_GPU_VIRTUAL_ADDRESS GetWidgetVirtualAddress() const { 
		if (mWidgetPool->GetBuffer()) return mWidgetPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return 0; }
	D3D12_GPU_VIRTUAL_ADDRESS GetParticleVirtualAddress() const {
		if (mParticlePool->GetBuffer()) return mParticlePool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return 0; }
	D3D12_GPU_VIRTUAL_ADDRESS GetSsaoVirtualAddress() const {
		if (mSsaoPool->GetBuffer()) return mSsaoPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return 0; }
 
public:
	static inline UINT processorCoreNum = 0;

	// 명령 할당자는 GPU가 명령들을 다 처리한 후 재설정해야한다.
	// 따라서 프레임마다 할당자가 필요하다.
	std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> mWorekrCmdLists;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> mWorkerCmdAllocs;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mFrameCmdLists[FRAME_PHASE];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mFrameCmdAllocs[FRAME_PHASE];
	std::vector<ID3D12CommandList*> mExecutableCmdLists;

	// Fence는 현재 울타리 지점까지의 명령들을 표시하는 값이다.
	// 이 값은 GPU가 아직 이 프레임 자원들을 사용하고 있는지 판정하는 용도로 쓰인다.
	UINT64 mFence = 0;

	// 상수 버퍼는 그것을 참조하는 명령들을 GPU가 다 처리한 후에 갱신해야 한다.
	// 따라서 프레임마다 상수 버퍼를 새로 만들어야 한다.
	std::unique_ptr<BufferMemoryPool<PassConstants>> mPassPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ObjectConstants>> mObjectPool = nullptr;
	std::unique_ptr<BufferMemoryPool<LightData>> mLightBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<MaterialData>> mMaterialBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<WidgetConstants>> mWidgetPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ParticleConstants>> mParticlePool = nullptr;
	std::unique_ptr<BufferMemoryPool<SsaoConstants>> mSsaoPool = nullptr;

	std::vector<std::unique_ptr<UploadBuffer<WidgetVertex>>> mWidgetVBs;
	std::vector<std::unique_ptr<UploadBuffer<ParticleVertex>>> mParticleVBs;
};