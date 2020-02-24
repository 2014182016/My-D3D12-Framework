#pragma once

#include "pch.h"
#include "Structures.h"
#include "Enums.h"
#include "BufferMemoryPool.hpp"

#define FRAME_PHASE 5

// CPU�� �� �������� ��� ��ϵ��� �����ϴ� �� �ʿ��� �ڿ�����
// ��ǥ�ϴ� ����ü
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

	// ��� �Ҵ��ڴ� GPU�� ��ɵ��� �� ó���� �� �缳���ؾ��Ѵ�.
	// ���� �����Ӹ��� �Ҵ��ڰ� �ʿ��ϴ�.
	std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> mWorekrCmdLists;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> mWorkerCmdAllocs;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mFrameCmdLists[FRAME_PHASE];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mFrameCmdAllocs[FRAME_PHASE];
	std::vector<ID3D12CommandList*> mExecutableCmdLists;

	// Fence�� ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϴ� ���̴�.
	// �� ���� GPU�� ���� �� ������ �ڿ����� ����ϰ� �ִ��� �����ϴ� �뵵�� ���δ�.
	UINT64 mFence = 0;

	// ��� ���۴� �װ��� �����ϴ� ��ɵ��� GPU�� �� ó���� �Ŀ� �����ؾ� �Ѵ�.
	// ���� �����Ӹ��� ��� ���۸� ���� ������ �Ѵ�.
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