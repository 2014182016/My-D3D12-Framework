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

		// ��Ƽ �������� ��� ���μ��� ���� �����Ѵ�.
		processorCoreNum = isMultiThread ? info.dwNumberOfProcessors : 1;
		// ��Ƽ �������� ��� ���� ������ ���� �����Ѵ�.
		framePhase = isMultiThread ? FRAME_COUNT : 1;

		isInitailize = true;
	}

	if (isMultiThread)
	{
		workerCmdAllocs.reserve(processorCoreNum); workerCmdAllocs.resize(processorCoreNum);
		worekrCmdLists.reserve(processorCoreNum); worekrCmdLists.resize(processorCoreNum);

		for (UINT i = 0; i < processorCoreNum; ++i)
		{
			// ��ɾ� �Ҵ��ڸ� �����Ѵ�.
			ThrowIfFailed(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(workerCmdAllocs[i].GetAddressOf())));

			// ��ɾ� ����Ʈ�� �����Ѵ�.
			ThrowIfFailed(device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				workerCmdAllocs[i].Get(),
				nullptr,
				IID_PPV_ARGS(worekrCmdLists[i].GetAddressOf())));

			// ����Ʈ�� �����ϱ� ���� ����Ʈ�� �ݴ´�.
			worekrCmdLists[i]->Close();
		}

		// ��Ƽ ������ �������� ���� ��ɾ� ����Ʈ�� �̸� ��Ƶд�.
		for (UINT i = 0; i < processorCoreNum; ++i)
			executableCmdLists.push_back(worekrCmdLists[i].Get());
		executableCmdLists.shrink_to_fit();
	}

	frameCmdAllocs.reserve(framePhase); frameCmdAllocs.resize(framePhase);
	frameCmdLists.reserve(framePhase); frameCmdLists.resize(framePhase);

	for (UINT i = 0; i < framePhase; ++i)
	{
		// ��ɾ� �Ҵ��ڸ� �����Ѵ�.
		ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(frameCmdAllocs[i].GetAddressOf())));

		// ��ɾ� ����Ʈ�� �����Ѵ�.
		ThrowIfFailed(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			frameCmdAllocs[i].Get(),
			nullptr,
			IID_PPV_ARGS(frameCmdLists[i].GetAddressOf())));

		// ����Ʈ�� �����ϱ� ���� ����Ʈ�� �ݴ´�.
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