#include <Framework/FrameResource.h>
#include <Framework/Defines.h>

FrameResource::FrameResource(ID3D12Device* device, bool isMultiThread,
	const UINT32 passCount, const UINT32 objectCount, const UINT32 lightCount,
	const UINT32 materialCount, const UINT32 widgetCount, const UINT32 particleCount)
{
	if (!isInitailize)
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		// 멀티 스레드의 경우 프로세서 수를 저장한다.
		processorCoreNum = isMultiThread ? info.dwNumberOfProcessors : 1;
		// 멀티 스레드의 경우 나눌 프레임 수를 저장한다.
		framePhase = isMultiThread ? FRAME_COUNT : 1;

		isInitailize = true;
	}

	if (isMultiThread)
	{
		workerCmdAllocs.reserve(processorCoreNum); workerCmdAllocs.resize(processorCoreNum);
		worekrCmdLists.reserve(processorCoreNum); worekrCmdLists.resize(processorCoreNum);

		for (UINT i = 0; i < processorCoreNum; ++i)
		{
			// 명령어 할당자를 생성한다.
			ThrowIfFailed(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(workerCmdAllocs[i].GetAddressOf())));

			// 명령어 리스트를 생성한다.
			ThrowIfFailed(device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				workerCmdAllocs[i].Get(),
				nullptr,
				IID_PPV_ARGS(worekrCmdLists[i].GetAddressOf())));

			// 리스트를 리셋하기 전에 리스트를 닫는다.
			worekrCmdLists[i]->Close();
		}

		// 멀티 스레드 렌더링을 위한 명령어 리스트를 미리 담아둔다.
		for (UINT i = 0; i < processorCoreNum; ++i)
			executableCmdLists.push_back(worekrCmdLists[i].Get());
		executableCmdLists.shrink_to_fit();
	}

	frameCmdAllocs.reserve(framePhase); frameCmdAllocs.resize(framePhase);
	frameCmdLists.reserve(framePhase); frameCmdLists.resize(framePhase);

	for (UINT i = 0; i < framePhase; ++i)
	{
		// 명령어 할당자를 생성한다.
		ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(frameCmdAllocs[i].GetAddressOf())));

		// 명령어 리스트를 생성한다.
		ThrowIfFailed(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			frameCmdAllocs[i].Get(),
			nullptr,
			IID_PPV_ARGS(frameCmdLists[i].GetAddressOf())));

		// 리스트를 리셋하기 전에 리스트를 닫는다.
		frameCmdLists[i]->Close();
	}

	passPool = std::make_unique<BufferMemoryPool<PassConstants>>(device, passCount, true);
	objectPool = std::make_unique<BufferMemoryPool<ObjectConstants>>(device, objectCount, true);
	lightBufferPool = std::make_unique<BufferMemoryPool<LightData>>(device, lightCount, false);
	materialBufferPool = std::make_unique<BufferMemoryPool<MaterialData>>(device, materialCount, false);
	ssaoPool = std::make_unique<BufferMemoryPool<SsaoConstants>>(device, 1, true);
	widgetPool = std::make_unique<BufferMemoryPool<ObjectConstants>>(device, widgetCount, true);
	particlePool = std::make_unique<BufferMemoryPool<ParticleConstants>>(device, particleCount, true);
	terrainPool = std::make_unique<BufferMemoryPool<TerrainConstants>>(device, 1, true);
	ssrPool = std::make_unique<BufferMemoryPool<SsrConstants>>(device, 1, true);
}

FrameResource::~FrameResource() 
{
	worekrCmdLists.clear();
	workerCmdAllocs.clear();
	frameCmdLists.clear();
	frameCmdAllocs.clear();
	executableCmdLists.clear();

	passPool = nullptr;
	objectPool = nullptr;
	lightBufferPool = nullptr;
	materialBufferPool = nullptr;
	ssaoPool = nullptr;
	widgetPool = nullptr;
	particlePool = nullptr;
	terrainPool = nullptr;
	ssrPool = nullptr;

	widgetVBs.clear();
}