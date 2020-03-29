#include <Framework/D3DApp.h>
#include <Framework/Ssao.h>
#include <Framework/Ssr.h>
#include <Framework/AssetManager.h>

using Microsoft::WRL::ComPtr;

D3DApp::D3DApp(HINSTANCE hInstance, const INT32 screenWidth, const INT32 screenHeight, 
	const std::wstring applicationName, const bool useWinApi)
	: WinApp(hInstance, screenWidth, screenHeight, applicationName, useWinApi) 
{
	options.reset();
}

D3DApp::~D3DApp()
{
	dxgiFactory = nullptr;
	swapChain = nullptr;
	d3dDevice = nullptr;

	fence = nullptr;
	commandQueue = nullptr;
	mainCommandAlloc = nullptr;
	mainCommandList = nullptr;

	depthStencilBuffer = nullptr;
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		swapChainBuffer[i] = nullptr;
	for (int i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
		deferredBuffer[i] = nullptr;

	rtvHeap = nullptr;
	dsvHeap = nullptr;
	cbvSrvUavDescriptorHeap = nullptr;

	d3dSound = nullptr;
	primarySoundBuffer = nullptr;
	listener = nullptr;

	rootSignatures.clear();
	pipelineStateObjects.clear();
	shaders.clear();
}


bool D3DApp::Initialize()
{
	if (!__super::Initialize())
		return false;

	if (!InitDirect3D())
		return false;

	OnResize(screenWidth, screenHeight);

	return true;
}

void D3DApp::Tick(float deltaTime)
{
	__super::Tick(deltaTime);
}

void D3DApp::OnDestroy()
{
	__super::OnDestroy();

	if (d3dDevice != nullptr)
		FlushCommandQueue();

	swapChain->SetFullscreenState(false, nullptr);
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps(const UINT32 shadowMapNum)
{
	// ����Ÿ�� �並 �����Ѵ�.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
#if defined(SSAO) || defined(SSR)
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT + DEFERRED_BUFFER_COUNT + 3; // Ssao Map, Ssr Map
#else
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT + DEFERRED_BUFFER_COUNT;
#endif
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));

	// ���� ���ٽ� �並 �����Ѵ�.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1 + shadowMapNum;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.GetAddressOf())));
}

void D3DApp::OnResize(const INT32 screenWidth, const INT32 screenHeight)
{
	__super::OnResize(screenWidth, screenHeight);

	if (!d3dDevice || !swapChain || !mainCommandAlloc)
		return;

	// ���ҽ��� �����ϱ� ���� ��ɵ��� ����.
	FlushCommandQueue();

	ThrowIfFailed(mainCommandList->Reset(mainCommandAlloc.Get(), nullptr));

	// ���ҽ��� ���� ����� ���� ������ ���ҽ��� �����Ѵ�.
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		swapChainBuffer[i].Reset();
	for (int i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
		deferredBuffer[i].Reset();
	depthStencilBuffer.Reset();

	// SwapChain�� Resize�Ѵ�.
	ThrowIfFailed(swapChain->ResizeBuffers(
		SWAP_CHAIN_BUFFER_COUNT,
		screenWidth, screenHeight,
		backBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	currentBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		// ���� Ÿ�� ���۸� �����Ѵ�.
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer[i])));

		// ������ �並 �����Ѵ�.
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = backBufferFormat;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		// ���� Ÿ�� �並 �����Ѵ�.
		d3dDevice->CreateRenderTargetView(swapChainBuffer[i].Get(), &rtvDesc, rtvHeapHandle);
		rtvHeapHandle.Offset(1, DescriptorSize::rtvDescriptorSize);
	}

	// G���ۿ� ���� �������� ���ҽ� �����ڸ� �����Ѵ�.
	D3D12_RESOURCE_DESC defferedBufferDesc;
	ZeroMemory(&defferedBufferDesc, sizeof(defferedBufferDesc));
	defferedBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	defferedBufferDesc.Alignment = 0;
	defferedBufferDesc.SampleDesc.Count = GetOptionEnabled(Option::Msaa) ? 4 : 1;
	defferedBufferDesc.SampleDesc.Quality = GetOptionEnabled(Option::Msaa) ? (msaaQuality - 1) : 0;
	defferedBufferDesc.MipLevels = 1;
	defferedBufferDesc.DepthOrArraySize = 1;
	defferedBufferDesc.Width = screenWidth;
	defferedBufferDesc.Height = screenHeight;
	defferedBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	defferedBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// G���ۿ� ���� �������� ���� Ÿ�� �����ڸ� �����Ѵ�.
	D3D12_RENDER_TARGET_VIEW_DESC defferedBufferRtvDesc;
	ZeroMemory(&defferedBufferRtvDesc, sizeof(defferedBufferRtvDesc));
	defferedBufferRtvDesc.Texture2D.MipSlice = 0;
	defferedBufferRtvDesc.Texture2D.PlaneSlice = 0;
	defferedBufferRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (UINT i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
	{
		CD3DX12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(deferredBufferFormats[i], (float*)&deferredBufferClearColors[i]);
		defferedBufferDesc.Format = deferredBufferFormats[i];
		
		// G���� ���ҽ��� �����Ѵ�.
		ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&defferedBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(deferredBuffer[i].GetAddressOf())));

		// G������ ����Ÿ�� �並 �����Ѵ�.
		d3dDevice->CreateRenderTargetView(deferredBuffer[i].Get(), &defferedBufferRtvDesc, rtvHeapHandle);
		rtvHeapHandle.Offset(1, DescriptorSize::rtvDescriptorSize);
	}

	// ���� ���ٽ� ���۸� �����ϱ� ���� ���ҽ� �����ڸ� �����Ѵ�.
	D3D12_RESOURCE_DESC depthStencilDesc = defferedBufferDesc;
	depthStencilDesc.Format = depthStencilFormat;
	depthStencilDesc.SampleDesc.Count = GetOptionEnabled(Option::Msaa) ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = GetOptionEnabled(Option::Msaa) ? (msaaQuality - 1) : 0;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// ���� ���ٽ� ���۸� �����ϱ� ���� ������ �並 �����Ѵ�.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = depthStencilFormat;
	dsvDesc.ViewDimension = GetOptionEnabled(Option::Fullscreen) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

	// ����⿡ ����ȭ�� ���� �����Ѵ�. ����ȭ�� ����� ����
	// �����ϴ� ����� ȣ���� �������� �ʴ� ȣ�⺸�� ���� �� �ִ�.
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = depthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	// ���� ���ٽ� ���۸� �����Ѵ�.
	ThrowIfFailed(d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));

	// ���� ���ٽ� ������ �並 �����Ѵ�.
	d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, GetDepthStencilView());

	// ���� ���۷μ� ����ϱ� ���� ���� ������ �Ѵ�.
	mainCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Resize ����� �����Ѵ�.
	ThrowIfFailed(mainCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mainCommandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Resize�� �Ϸ�� ������ ��ٸ���.
	FlushCommandQueue();

	// Viewport�� ScissorRect�� ������Ʈ�Ѵ�.
	screenViewport.TopLeftX = 0;
	screenViewport.TopLeftY = 0;
	screenViewport.Width = static_cast<float>(screenWidth);
	screenViewport.Height = static_cast<float>(screenHeight);
	screenViewport.MinDepth = 0.0f;
	screenViewport.MaxDepth = 1.0f;
	scissorRect = { 0, 0, screenWidth, screenHeight };
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// D3D12 ����� ���� Ȱ��ȭ�Ѵ�.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	CreateDevice();
	CreateCommandObjects();
	CreateSwapChain();
	CreateSoundBuffer();
	CreateRtvAndDsvDescriptorHeaps(LIGHT_NUM);
	CreateShadersAndInputLayout();

	// Alt-Enter�� ��Ȱ��ȭ�Ѵ�.
	dxgiFactory->MakeWindowAssociation(hMainWnd, DXGI_MWA_NO_ALT_ENTER);

#ifdef _DEBUG
	LogAdapters();
#endif

	return true;
}

void D3DApp::CreateDevice()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&d3dDevice));

	// D3D12Device�� ����� �Ϳ� �����Ͽ��ٸ� �ϵ���� �׷��� ��ɼ���
	// �䳻���� WARP(����Ʈ���� ���÷��� �����)�� �����Ѵ�.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&d3dDevice)));
	}

	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// �������� ũ��� CPU ���� �ٸ� �� �����Ƿ� �̸� �˾Ƴ��д�.
	DescriptorSize::rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DescriptorSize::dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	DescriptorSize::cbvSrvUavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// �ĸ� ���۸� ���� 4X MSAA ���� ���θ� �����Ѵ�.
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = backBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	msaaQuality = msQualityLevels.NumQualityLevels;
	assert(msaaQuality > 0 && "Unexpected MSAA quality level.");
}

void D3DApp::CreateCommandObjects()
{
	// ��ɾ� ť�� �����Ѵ�.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	// ��ɾ� �Ҵ��ڸ� �����Ѵ�.
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mainCommandAlloc.GetAddressOf())));

	// ��ɾ� ����Ʈ�� �����Ѵ�.
	ThrowIfFailed(d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mainCommandAlloc.Get(),
		nullptr,   
		IID_PPV_ARGS(mainCommandList.GetAddressOf())));

	// ���� ���·� �����Ѵ�. ���� ��� ����� ó�� ������ �� Reset�� ȣ���ϴµ�,
	// Reset�� ȣ���Ϸ��� CommandList�� ���� �־�� �ϱ� �����̴�.
	mainCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// SwapChain�� �����ϱ� ���� Fullscreen��带 �����ؾ߸� �Ѵ�.
	if (swapChain)
		swapChain->SetFullscreenState(false, nullptr);

	// SwapChain�� ����� ���� ���� SwapChain�� �����Ѵ�.
	swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = screenWidth;
	sd.BufferDesc.Height = screenHeight;
	sd.BufferDesc.RefreshRate.Numerator = 0; // ������� ����
	sd.BufferDesc.RefreshRate.Denominator = 0; // ������� ����
	sd.BufferDesc.Format = backBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1; // SwapChain���� ����ǥ��ȭ Ȱ��ȭ���� ����
	sd.SampleDesc.Quality = 0; // SwapChain���� ����ǥ��ȭ Ȱ��ȭ���� ����
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	sd.OutputWindow = GetMainWnd();
	sd.Windowed = !GetOptionEnabled(Option::Fullscreen);
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// ���� : ��ȯ �罽�� Command Queue�� �̿��ؼ� Flush�� �����Ѵ�.
	ThrowIfFailed(dxgiFactory->CreateSwapChain(
		commandQueue.Get(),
		&sd,
		swapChain.GetAddressOf()));
}

void D3DApp::CreateSoundBuffer()
{
	// Direct Sound ����
	ThrowIfFailed(DirectSoundCreate8(NULL, d3dSound.GetAddressOf(), NULL));
	ThrowIfFailed(d3dSound->SetCooperativeLevel(hMainWnd, DSSCL_PRIORITY));

	DSBUFFERDESC soundBufferDesc;
	soundBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	soundBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRL3D;
	soundBufferDesc.dwBufferBytes = 0;
	soundBufferDesc.dwReserved = 0;
	soundBufferDesc.lpwfxFormat = NULL;
	soundBufferDesc.guid3DAlgorithm = GUID_NULL;
	// �� ���� ���� ����
	ThrowIfFailed(d3dSound->CreateSoundBuffer(&soundBufferDesc, primarySoundBuffer.GetAddressOf(), NULL));

	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 2;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;
	ThrowIfFailed(primarySoundBuffer->SetFormat(&waveFormat));
	// ������ ����
	ThrowIfFailed(primarySoundBuffer->QueryInterface(IID_IDirectSound3DListener8, (void**)listener.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	// ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϵ��� ��Ÿ�� ���� ������Ų��.
	currentFence++;

	// �� ��Ÿ�� ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� ��Ÿ�� ������ GPU�� Signal() ��ɱ����� ��� ����� ó���ϱ�
	// �������� �������� �ʴ´�.
	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFence));

	// GPU�� �� ��Ÿ�� ���������� ��ɵ��� �Ϸ��� ������ ��ٸ���.
	if (fence->GetCompletedValue() < currentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// GPU�� ���� ��Ÿ�� ������ ���������� �̺�Ʈ�� �ߵ��Ѵ�.
		ThrowIfFailed(fence->SetEventOnCompletion(currentFence, eventHandle));

		// GPU�� ���� ��Ÿ�� ������ ���������� ���ϴ� �̺�Ʈ�� ��ٸ���.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::GetCurrentBackBuffer()const
{
	return swapChainBuffer[currentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		currentBackBuffer,
		DescriptorSize::rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDefferedBufferView(const UINT32 index) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		SWAP_CHAIN_BUFFER_COUNT + index,
		DescriptorSize::rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDepthStencilView() const
{
	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCpuSrv(const UINT32 index) const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srv.Offset(index, DescriptorSize::cbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE D3DApp::GetGpuSrv(const UINT32 index) const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(index, DescriptorSize::cbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDsv(const UINT32 index) const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart());
	dsv.Offset(index, DescriptorSize::dsvDescriptorSize);
	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3DApp::GetRtv(const UINT32 index) const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, DescriptorSize::rtvDescriptorSize);
	return rtv;
}


void D3DApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, backBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// nullptr�� �����ϸ� ����Ʈ�� ������ �����Ѵ�.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

void D3DApp::CreateRootSignatures(const UINT32 textureNum, const UINT32 shadowMapNum)
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ���(��� ����, �ؽ�ó, ǥ������� ��)��
	// �Էµȴٰ� ����Ѵ�. ��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�.

	///////////////////////////////////////// Common /////////////////////////////////////
	{
		CD3DX12_DESCRIPTOR_RANGE texTable[5];
		texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureNum, 0, 0);
		texTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shadowMapNum, 0, 1);
		texTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, DEFERRED_BUFFER_COUNT + 1, 0, 2); // Deferred Map + Depth Map
		texTable[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, DEFERRED_BUFFER_COUNT + 1, 2); // SSAO Map
		texTable[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, DEFERRED_BUFFER_COUNT + 2, 2); // Ssr Map

		std::array<CD3DX12_ROOT_PARAMETER, (int)RpCommon::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpCommon::Object].InitAsConstantBufferView(0);
		slotRootParameter[(int)RpCommon::Pass].InitAsConstantBufferView(1);
		slotRootParameter[(int)RpCommon::Light].InitAsShaderResourceView(0, 3);
		slotRootParameter[(int)RpCommon::Material].InitAsShaderResourceView(1, 3);
		slotRootParameter[(int)RpCommon::Texture].InitAsDescriptorTable(1, &texTable[0], D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[(int)RpCommon::ShadowMap].InitAsDescriptorTable(1, &texTable[1], D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[(int)RpCommon::GBuffer].InitAsDescriptorTable(1, &texTable[2], D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[(int)RpCommon::Ssao].InitAsDescriptorTable(1, &texTable[3], D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[(int)RpCommon::Ssr].InitAsDescriptorTable(1, &texTable[4], D3D12_SHADER_VISIBILITY_PIXEL);

		// �׷��� ���� ���α׷��� ����ϴ� ǥ��������� ���� �׸� ���� �����Ƿ�
		// �̸� ���� Root Signature�� ���Խ��� �д�.

		// �� ���͸�, ��ȯ ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		// �� ���͸�, ���� ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
			1, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		// ���� ���͸�, ��ȯ ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			2, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		// ���� ���͸�, ���� ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
			3, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		// ���� ���͸�, ��ȯ ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
			4, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
			0.0f,                             // mipLODBias
			8);                               // maxAnisotropy

		// ���� ���͸�, ���� ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
			5, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
			0.0f,                              // mipLODBias
			8);                                // maxAnisotropy

		// �׸��ڸ� ���� �� ǥ�������
		// �ϵ��� �׸��� �� �� �������� ������ �� �ְ� �ϱ� ���� ��
		// PCF(Percenatge Colser Filtering) : ���� ���� ���͸�
		const CD3DX12_STATIC_SAMPLER_DESC shadow(
			6, // shaderRegister
			D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
			0.0f,                               // mipLODBias
			16,                                 // maxAnisotropy
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

		std::array<CD3DX12_STATIC_SAMPLER_DESC, 7> staticSamplers = {
			pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap, anisotropicClamp, shadow };

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["Common"])));
	}

	///////////////////////////////////////// SSAO /////////////////////////////////////
	{
	CD3DX12_DESCRIPTOR_RANGE texTables[3];
	texTables[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);
	texTables[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	texTables[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);

	std::array<CD3DX12_ROOT_PARAMETER, (int)RpSsao::Count> slotRootParameter;

	// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
	slotRootParameter[(int)RpSsao::SsaoCB].InitAsConstantBufferView(0);
	slotRootParameter[(int)RpSsao::Pass].InitAsConstantBufferView(1);
	slotRootParameter[(int)RpSsao::Constants].InitAsConstants(1, 2);
	slotRootParameter[(int)RpSsao::BufferMap].InitAsDescriptorTable(1, &texTables[0], D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[(int)RpSsao::SsaoMap].InitAsDescriptorTable(1, &texTables[1], D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[(int)RpSsao::PositionMap].InitAsDescriptorTable(1, &texTables[2], D3D12_SHADER_VISIBILITY_PIXEL);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers = { pointClamp, linearClamp, depthMapSam, linearWrap };

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSignatures["Ssao"])));

	}

	///////////////////////////////////////// ParticleRender /////////////////////////////////////
	{
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureNum, 0, 1);

		std::array<CD3DX12_ROOT_PARAMETER, (int)RpParticleGraphics::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpParticleGraphics::ParticleCB].InitAsConstantBufferView(0);
		slotRootParameter[(int)RpParticleGraphics::Pass].InitAsConstantBufferView(1);
		slotRootParameter[(int)RpParticleGraphics::Buffer].InitAsShaderResourceView(0);
		slotRootParameter[(int)RpParticleGraphics::Texture].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		std::array<CD3DX12_STATIC_SAMPLER_DESC, 1> staticSamplers = { linearWrap };

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["ParticleRender"])));
	}

	///////////////////////////////////////// ParticleCompute /////////////////////////////////////
	{
		std::array<CD3DX12_ROOT_PARAMETER, (int)RpParticleCompute::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpParticleCompute::ParticleCB].InitAsConstantBufferView(0);
		slotRootParameter[(int)RpParticleCompute::Counter].InitAsUnorderedAccessView(0);
		slotRootParameter[(int)RpParticleCompute::Uav].InitAsUnorderedAccessView(1);
		slotRootParameter[(int)RpParticleCompute::Pass].InitAsConstantBufferView(1);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["ParticleCompute"])));
	}

	///////////////////////////////////////// TerrainRender /////////////////////////////////////
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2];
		texTables[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureNum, 0, 0);
		texTables[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 1);

		std::array<CD3DX12_ROOT_PARAMETER, (int)RpTerrainGraphics::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpTerrainGraphics::TerrainCB].InitAsConstantBufferView(0);
		slotRootParameter[(int)RpTerrainGraphics::Pass].InitAsConstantBufferView(1);
		slotRootParameter[(int)RpTerrainGraphics::Texture].InitAsDescriptorTable(1, &texTables[0], D3D12_SHADER_VISIBILITY_ALL);
		slotRootParameter[(int)RpTerrainGraphics::Srv].InitAsDescriptorTable(1, &texTables[1], D3D12_SHADER_VISIBILITY_ALL);
		slotRootParameter[(int)RpTerrainGraphics::Material].InitAsShaderResourceView(2, 1);

		// ���� ���͸�, ��ȯ ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		// ���� ���͸�, ��ȯ ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
			1, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
			0.0f,                             // mipLODBias
			8);                               // maxAnisotropy

		std::array<CD3DX12_STATIC_SAMPLER_DESC, 2> staticSamplers = { linearWrap, anisotropicWrap, };

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["TerrainRender"])));
	}

	///////////////////////////////////////// TerrainCompute /////////////////////////////////////
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2];
		texTables[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0);

		std::array<CD3DX12_ROOT_PARAMETER, (int)RpTerrainCompute::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpTerrainCompute::HeightMap].InitAsDescriptorTable(1, &texTables[0]);
		slotRootParameter[(int)RpTerrainCompute::Uav].InitAsDescriptorTable(1, &texTables[1]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["TerrainCompute"])));
	}

	///////////////////////////////////////// Ssr /////////////////////////////////////
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[3];
		texTables[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

		std::array<CD3DX12_ROOT_PARAMETER, (int)RpSsr::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpSsr::PositionMap].InitAsDescriptorTable(1, &texTables[0]);
		slotRootParameter[(int)RpSsr::NormalMap].InitAsDescriptorTable(1, &texTables[1]);
		slotRootParameter[(int)RpSsr::ColorMap].InitAsDescriptorTable(1, &texTables[2]);
		slotRootParameter[(int)RpSsr::SsrCB].InitAsConstantBufferView(0);
		slotRootParameter[(int)RpSsr::Pass].InitAsConstantBufferView(1);

		// ���� ���͸�, ��ȯ ��ǥ ���� ���
		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		std::array<CD3DX12_STATIC_SAMPLER_DESC, 1> staticSamplers = { linearWrap };

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["Ssr"])));
	}

	///////////////////////////////////////// Reflection /////////////////////////////////////
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[3];
		texTables[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0);
		texTables[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 3, 0);

		std::array<CD3DX12_ROOT_PARAMETER, (int)RpReflection::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpReflection::ColorMap].InitAsDescriptorTable(1, &texTables[0]);
		slotRootParameter[(int)RpReflection::BufferMap].InitAsDescriptorTable(1, &texTables[1]);
		slotRootParameter[(int)RpReflection::SsrMap].InitAsDescriptorTable(1, &texTables[2]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["Reflection"])));
	}

	///////////////////////////////////////// Blur /////////////////////////////////////
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2];
		texTables[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		std::array<CD3DX12_ROOT_PARAMETER, (int)RpBlur::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpBlur::BlurConstants].InitAsConstants(20, 0);
		slotRootParameter[(int)RpBlur::InputSrv].InitAsDescriptorTable(1, &texTables[0]);
		slotRootParameter[(int)RpBlur::OutputUav].InitAsDescriptorTable(1, &texTables[1]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["Blur"])));
	}

	///////////////////////////////////////// Debug /////////////////////////////////////
	{
		std::array<CD3DX12_ROOT_PARAMETER, (int)RpDebug::Count> slotRootParameter;

		// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
		slotRootParameter[(int)RpDebug::Pass].InitAsConstantBufferView(1);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(d3dDevice->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatures["Debug"])));
	}
}

void D3DApp::CreateDescriptorHeaps(const UINT32 textureNum, const UINT32 shadowMapNum, const UINT32 particleNum)
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDescriptorHeapDesc = {};
	cbvSrvUavDescriptorHeapDesc.NumDescriptors = textureNum +  shadowMapNum + (particleNum * 3) + DEFERRED_BUFFER_COUNT + SWAP_CHAIN_BUFFER_COUNT
		+ 1 + 3 + 4 + 1 + 4; // Depth Buffer, SsaoMaps, Terrain, SsrMap, BlurFilter
	cbvSrvUavDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvSrvUavDescriptorHeapDesc, IID_PPV_ARGS(&cbvSrvUavDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(cbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// �ؽ�ó�� Shader Resource View�� �����ϴ� ����ü
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = backBufferFormat;
	srvDesc.Texture2D.MipLevels = 1;

	DescriptorIndex::renderTargetHeapIndex = 0;
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		d3dDevice->CreateShaderResourceView(swapChainBuffer[i].Get(), &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	}

	DescriptorIndex::textureHeapIndex = DescriptorIndex::renderTargetHeapIndex + SWAP_CHAIN_BUFFER_COUNT;
	// �� �ؽ�ó���� ���� Shader Resource View�� �����Ѵ�.
	for (UINT i = 0; i < textureNum; ++i)
	{
		// texNames�� �ε����� Material�� ���� �ؽ�ó �ε����� �����ֱ� ���� ������� View�� �����Ѵ�.
		ID3D12Resource* resource = AssetManager::GetInstance()->GetTextureResource(i);

		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		d3dDevice->CreateShaderResourceView(resource, &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	}

	DescriptorIndex::deferredBufferHeapIndex = DescriptorIndex::textureHeapIndex + textureNum;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	for (UINT i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
	{
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = deferredBufferFormats[i];

		// G-Buffer�� ���Ǵ� ���� Ÿ�ٵ鿡 ���� SRV�� �����Ѵ�.
		d3dDevice->CreateShaderResourceView(deferredBuffer[i].Get(), &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	}

	DescriptorIndex::depthBufferHeapIndex = DescriptorIndex::deferredBufferHeapIndex + DEFERRED_BUFFER_COUNT;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	// G-Buffer�� ���Ǵ� ���� ���ۿ� ���� SRV�� �����Ѵ�.
	d3dDevice->CreateShaderResourceView(depthStencilBuffer.Get(), &srvDesc, hDescriptor);
	hDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
}

void D3DApp::CreateShadersAndInputLayout()
{
	// Direct3D���� �� ������ ������ �˷��ִ� InputLayout��
	// ���̴��� �������Ѵ�.

#ifdef SSAO
	D3D_SHADER_MACRO defines[] = {
		"SSAO", "1",
		NULL, NULL,
	};
#else
	D3D_SHADER_MACRO defines[] = {
		NULL, NULL,
	};
#endif

	shaders["ForwardVS"] = D3DUtil::CompileShader(L"Shaders\\Forward.hlsl", nullptr, "VS", "vs_5_1");
	shaders["ForwardPS"] = D3DUtil::CompileShader(L"Shaders\\Forward.hlsl", defines, "PS", "ps_5_1");

	shaders["OpaqueVS"] = D3DUtil::CompileShader(L"Shaders\\Opaque.hlsl", nullptr, "VS", "vs_5_1");
	shaders["OpaquePS"] = D3DUtil::CompileShader(L"Shaders\\Opaque.hlsl", nullptr, "PS", "ps_5_1");

	shaders["WireframeVS"] = D3DUtil::CompileShader(L"Shaders\\Wireframe.hlsl", nullptr, "VS", "vs_5_1");
	shaders["WireframePS"] = D3DUtil::CompileShader(L"Shaders\\Wireframe.hlsl", nullptr, "PS", "ps_5_1");

	shaders["SkyVS"] = D3DUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	shaders["SkyPS"] = D3DUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	shaders["ShadowMapVS"] = D3DUtil::CompileShader(L"Shaders\\ShadowMap.hlsl", nullptr, "VS", "vs_5_1");
	shaders["ShadowMapPS"] = D3DUtil::CompileShader(L"Shaders\\ShadowMap.hlsl", nullptr, "PS", "ps_5_1");

	shaders["WidgetVS"] = D3DUtil::CompileShader(L"Shaders\\Widget.hlsl", nullptr, "VS", "vs_5_1");
	shaders["WidgetPS"] = D3DUtil::CompileShader(L"Shaders\\Widget.hlsl", nullptr, "PS", "ps_5_1");

	shaders["ParticleRenderVS"] = D3DUtil::CompileShader(L"Shaders\\ParticleRender.hlsl", nullptr, "VS", "vs_5_1");
	shaders["ParticleRenderGS"] = D3DUtil::CompileShader(L"Shaders\\ParticleRender.hlsl", nullptr, "GS", "gs_5_1");
	shaders["ParticleRenderPS"] = D3DUtil::CompileShader(L"Shaders\\ParticleRender.hlsl", nullptr, "PS", "ps_5_1");

	shaders["ParticleUpdateCS"] = D3DUtil::CompileShader(L"Shaders\\ParticleCompute.hlsl", nullptr, "CS_Update", "cs_5_1");
	shaders["ParticleEmitCS"] = D3DUtil::CompileShader(L"Shaders\\ParticleCompute.hlsl", nullptr, "CS_Emit", "cs_5_1");

	shaders["BillboardVS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "VS", "vs_5_1");
	shaders["BillboardGS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "GS", "gs_5_1");
	shaders["BillboardPS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "PS", "ps_5_1");

	shaders["DebugVS"] = D3DUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "VS", "vs_5_1");
	shaders["DebugPS"] = D3DUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "PS", "ps_5_1");

	shaders["DiffuseMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "DiffuseMapDebugPS", "ps_5_1");
	shaders["SpecularMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "SpecularMapDebugPS", "ps_5_1");
	shaders["RoughnessMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "RoughnessMapDebugPS", "ps_5_1");
	shaders["NormalMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "NormalMapDebugPS", "ps_5_1");
	shaders["DepthMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "DepthMapDebugPS", "ps_5_1");
	shaders["PositionMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "PositionMapDebugPS", "ps_5_1");
	shaders["ShadowMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "ShadowMapDebugPS", "ps_5_1");
	shaders["SsaoMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "SsaoMapDebugPS", "ps_5_1");
	shaders["SsrMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "SsrMapDebugPS", "ps_5_1");
	shaders["BluredSsrMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "BluredSsrMapDebugPS", "ps_5_1");

	shaders["LightingPassVS"] = D3DUtil::CompileShader(L"Shaders\\LightingPass.hlsl", nullptr, "VS", "vs_5_1");
	shaders["LightingPassPS"] = D3DUtil::CompileShader(L"Shaders\\LightingPass.hlsl", defines, "PS", "ps_5_1");

	shaders["SsaoComputeVS"] = D3DUtil::CompileShader(L"Shaders\\SsaoCompute.hlsl", nullptr, "VS", "vs_5_1");
	shaders["SsaoComputePS"] = D3DUtil::CompileShader(L"Shaders\\SsaoCompute.hlsl", nullptr, "PS", "ps_5_1");
	shaders["SsaoBlurVS"] = D3DUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	shaders["SsaoBlurPS"] = D3DUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	shaders["TerrainVS"] = D3DUtil::CompileShader(L"Shaders\\TerrainRender.hlsl", nullptr, "VS", "vs_5_1");
	shaders["TerrainHS"] = D3DUtil::CompileShader(L"Shaders\\TerrainRender.hlsl", nullptr, "HS", "hs_5_1");
	shaders["TerrainDS"] = D3DUtil::CompileShader(L"Shaders\\TerrainRender.hlsl", nullptr, "DS", "ds_5_1");
	shaders["TerrainPS"] = D3DUtil::CompileShader(L"Shaders\\TerrainRender.hlsl", nullptr, "PS", "ps_5_1");
	shaders["TerrainWireframePS"] = D3DUtil::CompileShader(L"Shaders\\TerrainRender.hlsl", nullptr, "PS_Wireframe", "ps_5_1");
	shaders["TerrainStdDevCS"] = D3DUtil::CompileShader(L"Shaders\\TerrainCompute.hlsl", nullptr, "CS_StdDev", "cs_5_1");
	shaders["TerrainNormalCS"] = D3DUtil::CompileShader(L"Shaders\\TerrainCompute.hlsl", nullptr, "CS_Normal", "cs_5_1");

	shaders["SsrVS"] = D3DUtil::CompileShader(L"Shaders\\Ssr.hlsl", nullptr, "VS", "vs_5_1");
	shaders["SsrPS"] = D3DUtil::CompileShader(L"Shaders\\Ssr.hlsl", nullptr, "PS", "ps_5_1");
	shaders["ReflectionVS"] = D3DUtil::CompileShader(L"Shaders\\Reflection.hlsl", nullptr, "VS", "vs_5_1");
	shaders["ReflectionPS"] = D3DUtil::CompileShader(L"Shaders\\Reflection.hlsl", nullptr, "PS", "ps_5_1");

	shaders["BlurHorzCS"] = D3DUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "CS_BlurHorz", "cs_5_1");
	shaders["BlurVertCS"] = D3DUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "CS_BlurVert", "cs_5_1");

	defaultLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	billboardLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	widgetLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	lineLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	terrainLayout = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void D3DApp::CreatePSOs()
{
	// Direct3D���� ����� ������������ ���´� PSO��ü�� ���� �̸� �����ȴ�.
	// �� ��� ���� �ϳ��� ����ü�� �����ϴ� ������ Direct3D�� ��� ���°�
	// ȣȯ�Ǵ��� �̸� ������ �� ������ ����̹��� �ϵ���� ������ ���α׷�����
	// ���� ��� �ڵ带 �̸� ������ �� �ֱ� �����̴�.

	// PSO for Forward
	D3D12_GRAPHICS_PIPELINE_STATE_DESC forwardPsoDesc;
	ZeroMemory(&forwardPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	forwardPsoDesc.InputLayout = { defaultLayout.data(), (UINT)defaultLayout.size() };
	forwardPsoDesc.pRootSignature = rootSignatures["Common"].Get();
	forwardPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["ForwardVS"]->GetBufferPointer()),
		shaders["ForwardVS"]->GetBufferSize()
	};
	forwardPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["ForwardPS"]->GetBufferPointer()),
		shaders["ForwardPS"]->GetBufferSize()
	};
	forwardPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	forwardPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	forwardPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	forwardPsoDesc.SampleMask = UINT_MAX;
	forwardPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	forwardPsoDesc.NumRenderTargets = 1;
	forwardPsoDesc.RTVFormats[0] = backBufferFormat;
	forwardPsoDesc.SampleDesc.Count = 1;
	forwardPsoDesc.SampleDesc.Quality = 0;
	forwardPsoDesc.DSVFormat = depthStencilFormat;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&forwardPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Forward"])));


	// PSO for G-Buffer
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gBufferPsoDesc = forwardPsoDesc;
	gBufferPsoDesc.NumRenderTargets = DEFERRED_BUFFER_COUNT;
	for (UINT i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
	{
		gBufferPsoDesc.RTVFormats[i] = deferredBufferFormats[i];
	}

	// PSO for Opaque
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = gBufferPsoDesc;
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["OpaqueVS"]->GetBufferPointer()),
		shaders["OpaqueVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["OpaquePS"]->GetBufferPointer()),
		shaders["OpaquePS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Opaque"])));

	// PSO for Billborad
	D3D12_GRAPHICS_PIPELINE_STATE_DESC billboardPsoDesc = gBufferPsoDesc;
	billboardPsoDesc.InputLayout = { billboardLayout.data(), (UINT)billboardLayout.size() };
	billboardPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["BillboardVS"]->GetBufferPointer()),
		shaders["BillboardVS"]->GetBufferSize()
	};
	billboardPsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(shaders["BillboardGS"]->GetBufferPointer()),
		shaders["BillboardGS"]->GetBufferSize()
	};
	billboardPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["BillboardPS"]->GetBufferPointer()),
		shaders["BillboardPS"]->GetBufferSize()
	};
	billboardPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	billboardPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// MSAA�� Ȱ��ȭ�Ǿ� �ִٸ� �ϵ����� ���� ���� �̿��ؼ� MSAA�� ���� ���� �����Ͽ� ���ø��Ѵ�.
	billboardPsoDesc.BlendState.AlphaToCoverageEnable = GetOptionEnabled(Option::Msaa);
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&billboardPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Billborad"])));


	// PSO for AlphaTested
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = gBufferPsoDesc;
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["AlphaTested"])));


	// PSO for Wireframe 
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = forwardPsoDesc;
	opaqueWireframePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["WireframeVS"]->GetBufferPointer()),
		shaders["WireframeVS"]->GetBufferSize()
	};
	opaqueWireframePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["WireframePS"]->GetBufferPointer()),
		shaders["WireframePS"]->GetBufferSize()
	};
	opaqueWireframePsoDesc.DepthStencilState.DepthEnable = false;
	opaqueWireframePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaqueWireframePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Wireframe"])));


	// PSO for Debug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = opaqueWireframePsoDesc;
	debugPsoDesc.pRootSignature = rootSignatures["Debug"].Get();
	debugPsoDesc.InputLayout = { lineLayout.data(), (UINT)lineLayout.size() };
	debugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["DebugVS"]->GetBufferPointer()),
		shaders["DebugVS"]->GetBufferSize()
	};
	debugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["DebugPS"]->GetBufferPointer()),
		shaders["DebugPS"]->GetBufferSize()
	};
	debugPsoDesc.DepthStencilState.DepthEnable = true;
	debugPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Debug"])));


	// PSO for Transparent
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = forwardPsoDesc;
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	// �������� ��ü���� ������ �����Ǿ� ���̰� �Ϸ��� ���� ���⸦ ��Ȱ��ȭ�ϰų� �������� ��ü�� �����Ͽ� �׷��� �Ѵ�.
	// D3D12_DEPTH_WRITE_MASK_ZERO�� ���� ��ϸ� ��Ȱ��ȭ�� �� ���� �б�� ���� ������ ������ Ȱ��ȭ�Ѵ�.
	transparentPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Transparent"])));


	// PSO for ParticleRender
	D3D12_GRAPHICS_PIPELINE_STATE_DESC particleRenderPsoDesc = transparentPsoDesc;
	particleRenderPsoDesc.InputLayout = { nullptr, 0 };
	particleRenderPsoDesc.pRootSignature = rootSignatures["ParticleRender"].Get();
	particleRenderPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["ParticleRenderVS"]->GetBufferPointer()),
		shaders["ParticleRenderVS"]->GetBufferSize()
	};
	particleRenderPsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(shaders["ParticleRenderGS"]->GetBufferPointer()),
		shaders["ParticleRenderGS"]->GetBufferSize()
	};
	particleRenderPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["ParticleRenderPS"]->GetBufferPointer()),
		shaders["ParticleRenderPS"]->GetBufferSize()
	};
	particleRenderPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	particleRenderPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&particleRenderPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["ParticleRender"])));


	// PSO for ParticleUpdate
	D3D12_COMPUTE_PIPELINE_STATE_DESC particleUpdatePsoDesc = {};
	particleUpdatePsoDesc.pRootSignature = rootSignatures["ParticleCompute"].Get();
	particleUpdatePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shaders["ParticleUpdateCS"]->GetBufferPointer()),
		shaders["ParticleUpdateCS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&particleUpdatePsoDesc, IID_PPV_ARGS(&pipelineStateObjects["ParticleUpdate"])));


	// PSO for ParticleEmit
	D3D12_COMPUTE_PIPELINE_STATE_DESC particleEmitPsoDesc = {};
	particleEmitPsoDesc.pRootSignature = rootSignatures["ParticleCompute"].Get();
	particleEmitPsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shaders["ParticleEmitCS"]->GetBufferPointer()),
		shaders["ParticleEmitCS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&particleEmitPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["ParticleEmit"])));


	// PSO for Sky
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = forwardPsoDesc;
	// �ɸ޶�� �ϴ� �� ���ο� �����Ƿ� �ĸ� ������ ����.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// ���� �Լ��� LESS�� �ƴ϶� LESS_EQUAL�� �����Ѵ�.
	// �̷��� ���� ������, ���� ���۸� 1�� ����� ��� z = 1(NDC)����
	// ����ȭ�� ���� ���� ���� ������ �����Ѵ�.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["SkyVS"]->GetBufferPointer()),
		shaders["SkyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["SkyPS"]->GetBufferPointer()),
		shaders["SkyPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Sky"])));


	// PSO for Shadow
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = forwardPsoDesc;
	// Bias = (float)DepthBias * r + SlopeScaledDepthBias * MaxDepthSlope;
	// ���⼭ r�� ���� ���� ������ float32�� ��ȯ���� ��, ǥ�� ������ 0���� ū �ּڰ��̴�.
	// 24��Ʈ ���� ������ ��� r = 1 /2^24�̴�.
	// ex. DepthBias = 100000 -> ���� ���� ����ġ = 100000 / 2^24 = 0.006
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["ShadowMapVS"]->GetBufferPointer()),
		shaders["ShadowMapVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["ShadowMapPS"]->GetBufferPointer()),
		shaders["ShadowMapPS"]->GetBufferSize()
	};
	// �׸��� �� �н����� ���� ����� ����.
	// �׸��� ���� ���� ���۸��� ����ϹǷ� ���� ����� ������� �������ν�
	// ������ ����ȭ�� ������ �� �ִ�.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["ShadowMap"])));

#ifdef SSAO
	// PSO for Ssao
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = forwardPsoDesc;
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = rootSignatures["Ssao"].Get();
	ssaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["SsaoComputeVS"]->GetBufferPointer()),
		shaders["SsaoComputeVS"]->GetBufferSize()
	};
	ssaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["SsaoComputePS"]->GetBufferPointer()),
		shaders["SsaoComputePS"]->GetBufferSize()
	};
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.RTVFormats[0] = Ssao::ambientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["SsaoCompute"])));

	// PSO for SsaoBlur
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
	ssaoBlurPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["SsaoBlurVS"]->GetBufferPointer()),
		shaders["SsaoBlurVS"]->GetBufferSize()
	};
	ssaoBlurPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["SsaoBlurPS"]->GetBufferPointer()),
		shaders["SsaoBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["SsaoBlur"])));
#endif


	// PSO for Widget
	D3D12_GRAPHICS_PIPELINE_STATE_DESC widgetPsoDesc = forwardPsoDesc;
	widgetPsoDesc.InputLayout = { widgetLayout.data(), (UINT)widgetLayout.size() };
	widgetPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["WidgetVS"]->GetBufferPointer()),
		shaders["WidgetVS"]->GetBufferSize()
	};
	widgetPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["WidgetPS"]->GetBufferPointer()),
		shaders["WidgetPS"]->GetBufferSize()
	};
	widgetPsoDesc.DepthStencilState.DepthEnable = false;
	widgetPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	widgetPsoDesc.DepthStencilState.StencilEnable = false;
	widgetPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&widgetPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Widget"])));

	// PSO for DiffuseMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC diffuseMapDebugPsoDesc = widgetPsoDesc;
	diffuseMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["DiffuseMapDebugPS"]->GetBufferPointer()),
		shaders["DiffuseMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&diffuseMapDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["DiffuseMapDebug"])));


	// PSO for SpecularMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC specularMapDebugPsoDesc = widgetPsoDesc;
	specularMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["SpecularMapDebugPS"]->GetBufferPointer()),
		shaders["SpecularMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&specularMapDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["SpecularMapDebug"])));


	// PSO for RoughnessMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC roughnessMapDebugDebugPsoDesc = widgetPsoDesc;
	roughnessMapDebugDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["RoughnessMapDebugPS"]->GetBufferPointer()),
		shaders["RoughnessMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&roughnessMapDebugDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["RoughnessMapDebug"])));


	// PSO for NormalMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalMapDebuggPsoDesc = widgetPsoDesc;
	normalMapDebuggPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["NormalMapDebugPS"]->GetBufferPointer()),
		shaders["NormalMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&normalMapDebuggPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["NormalMapDebug"])));


	// PSO for DepthMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC depthMapDebugPsoDesc = widgetPsoDesc;
	depthMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["DepthMapDebugPS"]->GetBufferPointer()),
		shaders["DepthMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&depthMapDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["DepthMapDebug"])));


	// PSO for ShadowMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowMapDebugPsoDesc = widgetPsoDesc;
	shadowMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["ShadowMapDebugPS"]->GetBufferPointer()),
		shaders["ShadowMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&shadowMapDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["ShadowMapDebug"])));


	// PSO for SsaoMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoMapDebugPsoDesc = widgetPsoDesc;
	ssaoMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["SsaoMapDebugPS"]->GetBufferPointer()),
		shaders["SsaoMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&ssaoMapDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["SsaoMapDebug"])));


	// PSO for SsrMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssrMapDebugPsoDesc = widgetPsoDesc;
	ssrMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["SsrMapDebugPS"]->GetBufferPointer()),
		shaders["SsrMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&ssrMapDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["SsrMapDebug"])));


	// PSO for BluredSsrMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC bluredSsrMapDebugPsoDesc = widgetPsoDesc;
	bluredSsrMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["BluredSsrMapDebugPS"]->GetBufferPointer()),
		shaders["BluredSsrMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&bluredSsrMapDebugPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["BluredSsrMapDebug"])));


	// PSO for LightingPass
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lightingPassPsoDesc = widgetPsoDesc;
	lightingPassPsoDesc.InputLayout = { nullptr, 0 };
	lightingPassPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["LightingPassVS"]->GetBufferPointer()),
		shaders["LightingPassVS"]->GetBufferSize()
	};
	lightingPassPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["LightingPassPS"]->GetBufferPointer()),
		shaders["LightingPassPS"]->GetBufferSize()
	};
	lightingPassPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&lightingPassPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["LightingPass"])));


	// PSO for TerrainPass
	D3D12_GRAPHICS_PIPELINE_STATE_DESC terrainPsoDesc = gBufferPsoDesc;
	terrainPsoDesc.InputLayout = { terrainLayout.data(), (UINT)terrainLayout.size() };
	terrainPsoDesc.pRootSignature = rootSignatures["TerrainRender"].Get();
	terrainPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainVS"]->GetBufferPointer()),
		shaders["TerrainVS"]->GetBufferSize()
	};
	terrainPsoDesc.HS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainHS"]->GetBufferPointer()),
		shaders["TerrainHS"]->GetBufferSize()
	};
	terrainPsoDesc.DS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainDS"]->GetBufferPointer()),
		shaders["TerrainDS"]->GetBufferSize()
	};
	terrainPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainPS"]->GetBufferPointer()),
		shaders["TerrainPS"]->GetBufferSize()
	};
	terrainPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	terrainPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&terrainPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["TerrainRender"])));


	// PSO for TerrainWireframePass
	D3D12_GRAPHICS_PIPELINE_STATE_DESC terrainWireframePsoDesc = opaqueWireframePsoDesc;
	terrainWireframePsoDesc.InputLayout = { terrainLayout.data(), (UINT)terrainLayout.size() };
	terrainWireframePsoDesc.pRootSignature = rootSignatures["TerrainRender"].Get();
	terrainWireframePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainVS"]->GetBufferPointer()),
		shaders["TerrainVS"]->GetBufferSize()
	};
	terrainWireframePsoDesc.HS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainHS"]->GetBufferPointer()),
		shaders["TerrainHS"]->GetBufferSize()
	};
	terrainWireframePsoDesc.DS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainDS"]->GetBufferPointer()),
		shaders["TerrainDS"]->GetBufferSize()
	};
	terrainWireframePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainWireframePS"]->GetBufferPointer()),
		shaders["TerrainWireframePS"]->GetBufferSize()
	};
	terrainWireframePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&terrainWireframePsoDesc, IID_PPV_ARGS(&pipelineStateObjects["TerrainWireframe"])));


	// PSO for TerrainStdDev
	D3D12_COMPUTE_PIPELINE_STATE_DESC terrainComputePsoDesc = {};
	terrainComputePsoDesc.pRootSignature = rootSignatures["TerrainCompute"].Get();
	terrainComputePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainStdDevCS"]->GetBufferPointer()),
		shaders["TerrainStdDevCS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&terrainComputePsoDesc, IID_PPV_ARGS(&pipelineStateObjects["TerrainStdDev"])));


	// PSO for TerrainNormal
	D3D12_COMPUTE_PIPELINE_STATE_DESC terrainNormalPsoDesc = {};
	terrainNormalPsoDesc.pRootSignature = rootSignatures["TerrainCompute"].Get();
	terrainNormalPsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shaders["TerrainNormalCS"]->GetBufferPointer()),
		shaders["TerrainNormalCS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&terrainNormalPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["TerrainNormal"])));


	// PSO for Ssr
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssrPsoDesc = forwardPsoDesc;
	ssrPsoDesc.InputLayout = { nullptr, 0 };
	ssrPsoDesc.pRootSignature = rootSignatures["Ssr"].Get();
	ssrPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["SsrVS"]->GetBufferPointer()),
		shaders["SsrVS"]->GetBufferSize()
	};
	ssrPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["SsrPS"]->GetBufferPointer()),
		shaders["SsrPS"]->GetBufferSize()
	};
	ssrPsoDesc.DepthStencilState.DepthEnable = false;
	ssrPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssrPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ssrPsoDesc.RTVFormats[0] = Ssr::ssrMapFormat;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&ssrPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Ssr"])));


	// PSO for Reflection
	D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectionPsoDesc = lightingPassPsoDesc;
	reflectionPsoDesc.pRootSignature = rootSignatures["Reflection"].Get();
	reflectionPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["ReflectionVS"]->GetBufferPointer()),
		shaders["ReflectionVS"]->GetBufferSize()
	};
	reflectionPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["ReflectionPS"]->GetBufferPointer()),
		shaders["ReflectionPS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&reflectionPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["Reflection"])));


	// PSO for BlurHorz
	D3D12_COMPUTE_PIPELINE_STATE_DESC blurHorzPsoDesc = {};
	blurHorzPsoDesc.pRootSignature = rootSignatures["Blur"].Get();
	blurHorzPsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shaders["BlurHorzCS"]->GetBufferPointer()),
		shaders["BlurHorzCS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&blurHorzPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["BlurHorz"])));

	// PSO for BlurVert
	D3D12_COMPUTE_PIPELINE_STATE_DESC blurVertPsoDesc = {};
	blurVertPsoDesc.pRootSignature = rootSignatures["Blur"].Get();
	blurVertPsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(shaders["BlurVertCS"]->GetBufferPointer()),
		shaders["BlurVertCS"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateComputePipelineState(&blurVertPsoDesc, IID_PPV_ARGS(&pipelineStateObjects["BlurVert"])));
}

void D3DApp::SetBackgroundColor(const float r, const float g, const float b, const float a)
{
	backBufferClearColor.x = r;
	backBufferClearColor.y = g;
	backBufferClearColor.z = b;
	backBufferClearColor.w = a;
}

void D3DApp::Reset(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc)
{
	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	// �缳���� GPU�� ���� ��ɸ�ϵ��� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(cmdAlloc->Reset());

	// ��� ����� ExcuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�
	// ��� ����� �缳���� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(cmdList->Reset(cmdAlloc, nullptr));
}

void D3DApp::ApplyOption(const Option option)
{
	bool optionState = GetOptionEnabled(option);
}

void D3DApp::SetOptionEnabled(const Option option, const bool value)
{
	options.set((int)option, value);

	ApplyOption(option);
}

void D3DApp::SwitchOptionEnabled(const Option option)
{
	options.flip((int)option);

	ApplyOption(option);
}

bool D3DApp::GetOptionEnabled(const Option option)
{
	return options.test((int)option);
}