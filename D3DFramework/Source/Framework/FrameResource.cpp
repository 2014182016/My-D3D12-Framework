#include "pch.h"
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, bool isMultiThread,
	UINT passCount, UINT objectCount, UINT lightCount, UINT materialCount, UINT widgetCount, UINT particleCount)
{
	if (!isInitailize)
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		processorCoreNum = isMultiThread ? info.dwNumberOfProcessors : 1;

		framePhase = isMultiThread ? 2 : 1;

		isInitailize = true;
	}

	if (isMultiThread)
	{
		mWorkerCmdAllocs.reserve(processorCoreNum); mWorkerCmdAllocs.resize(processorCoreNum);
		mWorekrCmdLists.reserve(processorCoreNum); mWorekrCmdLists.resize(processorCoreNum);

		for (UINT i = 0; i < processorCoreNum; ++i)
		{
			ThrowIfFailed(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(mWorkerCmdAllocs[i].GetAddressOf())));

			ThrowIfFailed(device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				mWorkerCmdAllocs[i].Get(),
				nullptr,
				IID_PPV_ARGS(mWorekrCmdLists[i].GetAddressOf())));

			mWorekrCmdLists[i]->Close();
		}

		for (UINT i = 0; i < processorCoreNum; ++i)
			mExecutableCmdLists.push_back(mWorekrCmdLists[i].Get());
		mExecutableCmdLists.shrink_to_fit();
	}

	mFrameCmdAllocs.reserve(framePhase); mFrameCmdAllocs.resize(framePhase);
	mFrameCmdLists.reserve(framePhase); mFrameCmdLists.resize(framePhase);

	for (UINT i = 0; i < framePhase; ++i)
	{
		ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(mFrameCmdAllocs[i].GetAddressOf())));

		ThrowIfFailed(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mFrameCmdAllocs[i].Get(),
			nullptr,
			IID_PPV_ARGS(mFrameCmdLists[i].GetAddressOf())));

		mFrameCmdLists[i]->Close();
	}

	mPassPool = std::make_unique<BufferMemoryPool<PassConstants>>(device, passCount, true);
	mObjectPool = std::make_unique<BufferMemoryPool<ObjectConstants>>(device, objectCount, true);
	mLightBufferPool = std::make_unique<BufferMemoryPool<LightData>>(device, lightCount, false);
	mMaterialBufferPool = std::make_unique<BufferMemoryPool<MaterialData>>(device, materialCount, false);
	mSsaoPool = std::make_unique<BufferMemoryPool<SsaoConstants>>(device, 1, true);
	mWidgetPool = std::make_unique<BufferMemoryPool<ObjectConstants>>(device, widgetCount, true);
	mParticlePool = std::make_unique<BufferMemoryPool<ParticleConstants>>(device, particleCount, true);
	mTerrainPool = std::make_unique<BufferMemoryPool<TerrainConstants>>(device, 1, true);
}

FrameResource::~FrameResource() 
{
	mWorekrCmdLists.clear();
	mWorkerCmdAllocs.clear();
	mFrameCmdLists.clear();
	mFrameCmdAllocs.clear();
	mExecutableCmdLists.clear();

	mPassPool = nullptr;
	mObjectPool = nullptr;
	mLightBufferPool = nullptr;
	mMaterialBufferPool = nullptr;
	mSsaoPool = nullptr;
	mWidgetPool = nullptr;
	mParticlePool = nullptr;
	mTerrainPool = nullptr;

	mWidgetVBs.clear();
}