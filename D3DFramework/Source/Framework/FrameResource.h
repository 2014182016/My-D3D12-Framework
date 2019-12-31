#pragma once

#include "pch.h"
#include "Structures.h"
#include "Enums.h"
#include "BufferMemoryPool.hpp"

// CPU�� �� �������� ��� ��ϵ��� �����ϴ� �� �ʿ��� �ڿ�����
// ��ǥ�ϴ� ����ü
struct FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT lightCount, UINT materialCount,
		UINT widgetCount, UINT particleCount)
	{
		ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(mCmdListAlloc.GetAddressOf())));

		mPassPool = std::make_unique<BufferMemoryPool<PassConstants>>(device, passCount, true);
		mObjectPool = std::make_unique<BufferMemoryPool<ObjectConstants>>(device, objectCount, true);
		mLightBufferPool = std::make_unique<BufferMemoryPool<LightData>>(device, lightCount, false);
		mMaterialBufferPool = std::make_unique<BufferMemoryPool<MaterialData>>(device, materialCount, false);
		mDebugPool = std::make_unique<BufferMemoryPool<DebugConstants>>(device, objectCount, true);
		mWidgetPool = std::make_unique<BufferMemoryPool<WidgetConstants>>(device, widgetCount, true);
		mParticlePool = std::make_unique<BufferMemoryPool<ParticleConstants>>(device, particleCount, true);
	}
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource() { }
 
public:
	// ��� �Ҵ��ڴ� GPU�� ��ɵ��� �� ó���� �� �缳���ؾ��Ѵ�.
	// ���� �����Ӹ��� �Ҵ��ڰ� �ʿ��ϴ�.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdListAlloc;

	// Fence�� ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϴ� ���̴�.
	// �� ���� GPU�� ���� �� ������ �ڿ����� ����ϰ� �ִ��� �����ϴ� �뵵�� ���δ�.
	UINT64 mFence = 0;

	// ��� ���۴� �װ��� �����ϴ� ��ɵ��� GPU�� �� ó���� �Ŀ� �����ؾ� �Ѵ�.
	// ���� �����Ӹ��� ��� ���۸� ���� ������ �Ѵ�.
	std::unique_ptr<BufferMemoryPool<PassConstants>> mPassPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ObjectConstants>> mObjectPool = nullptr;
	std::unique_ptr<BufferMemoryPool<LightData>> mLightBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<MaterialData>> mMaterialBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<DebugConstants>> mDebugPool = nullptr;
	std::unique_ptr<BufferMemoryPool<WidgetConstants>> mWidgetPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ParticleConstants>> mParticlePool = nullptr;

	std::vector<std::unique_ptr<UploadBuffer<WidgetVertex>>> mWidgetVBs;
	std::vector<std::unique_ptr<UploadBuffer<ParticleVertex>>> mParticleVBs;
};