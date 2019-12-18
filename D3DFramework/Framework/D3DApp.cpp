#include "pch.h"
#include "D3DApp.h"
#include "D3DUtil.h"

using Microsoft::WRL::ComPtr;

D3DApp::D3DApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName)
	: WinApp(hInstance, screenWidth, screenHeight, applicationName) { }

D3DApp::~D3DApp() { }


void D3DApp::Set4xMsaaState(bool value)
{
	mOptions.set((int)Option::Msaa, value);

	OnResize(mScreenWidth, mScreenHeight);
}

void D3DApp::SetFullscreenState(bool value)
{
	mOptions.set((int)Option::Fullscreen, value);

	ThrowIfFailed(mSwapChain->SetFullscreenState(value, nullptr));
	OnResize(mScreenWidth, mScreenHeight);
}

bool D3DApp::Initialize()
{
	if (!__super::Initialize())
		return false;

	if (!InitDirect3D())
		return false;

	OnResize(mScreenWidth, mScreenHeight);

	return true;
}

void D3DApp::OnDestroy()
{
	__super::OnDestroy();

	if (md3dDevice != nullptr)
		FlushCommandQueue();

	mSwapChain->SetFullscreenState(false, nullptr);
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT + 1; // msaa�� ���� �߰� ���� Ÿ��
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::OnResize(int screenWidth, int screenHeight)
{
	__super::OnResize(screenWidth, screenHeight);

	if (!md3dDevice || !mSwapChain || !mDirectCmdListAlloc)
		return;

	/*
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);
	*/

	// ���ҽ��� �����ϱ� ���� ��ɵ��� ����.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// ���ҽ��� ���� ����� ���� ������ ���ҽ��� �����Ѵ�.
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// SwapChain�� Resize�Ѵ�.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SWAP_CHAIN_BUFFER_COUNT,
		mScreenWidth, mScreenHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrentBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mScreenWidth;
	depthStencilDesc.Height = mScreenHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = GetOptionEnabled(Option::Msaa) ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = GetOptionEnabled(Option::Msaa) ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.ViewDimension = GetOptionEnabled(Option::Fullscreen) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

	// ����⿡ ����ȭ�� ���� �����Ѵ�. ����ȭ�� ����� ����
	// �����ϴ� ����� ȣ���� �������� �ʴ� ȣ�⺸�� ���� �� �ִ�.
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, GetDepthStencilView());

	// ���� ���۷μ� ����ϱ� ���� ���� ������ �Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Resize ����� �����Ѵ�.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Resize�� �Ϸ�� ������ ��ٸ���.
	FlushCommandQueue();

	// Viewport�� ScissorRect�� ������Ʈ�Ѵ�.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mScreenWidth);
	mScreenViewport.Height = static_cast<float>(mScreenHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mScreenWidth, mScreenHeight };
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

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// D3D12Device�� ����� �Ϳ� �����Ͽ��ٸ� �ϵ���� �׷��� ��ɼ���
	// �䳻���� WARP(����Ʈ���� ���÷��� �����)�� �����Ѵ�.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	// �������� ũ��� CPU ���� �ٸ� �� �����Ƿ� �̸� �˾Ƴ��д�.
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// �ĸ� ���۸� ���� 4X MSAA ���� ���θ� �����Ѵ�.
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	// Alt-Enter�� ��Ȱ��ȭ�Ѵ�.
	mdxgiFactory->MakeWindowAssociation(mhMainWnd, DXGI_MWA_NO_ALT_ENTER);


	// Direct Sound ����
	ThrowIfFailed(DirectSoundCreate8(NULL, md3dSound.GetAddressOf(), NULL));
	ThrowIfFailed(md3dSound->SetCooperativeLevel(mhMainWnd, DSSCL_PRIORITY));

	DSBUFFERDESC soundBufferDesc;
	soundBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	soundBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRL3D;
	soundBufferDesc.dwBufferBytes = 0;
	soundBufferDesc.dwReserved = 0;
	soundBufferDesc.lpwfxFormat = NULL;
	soundBufferDesc.guid3DAlgorithm = GUID_NULL;
	// �� ���� ���� ����
	ThrowIfFailed(md3dSound->CreateSoundBuffer(&soundBufferDesc, mPrimarySoundBuffer.GetAddressOf(), NULL));

	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 2;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;
	ThrowIfFailed(mPrimarySoundBuffer->SetFormat(&waveFormat));
	// ������ ����
	ThrowIfFailed(mPrimarySoundBuffer->QueryInterface(IID_IDirectSound3DListener8, (void**)mListener.GetAddressOf()));

	return true;
}

void D3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(),
		nullptr,   
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// ���� ���·� �����Ѵ�. ���� ��� ����� ó�� ������ �� Reset�� ȣ���ϴµ�,
	// Reset�� ȣ���Ϸ��� CommandList�� ���� �־�� �ϱ� �����̴�.
	mCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// SwapChain�� �����ϱ� ���� Fullscreen��带 �����ؾ߸� �Ѵ�.
	if (mSwapChain)
		mSwapChain->SetFullscreenState(false, nullptr);

	// SwapChain�� ����� ���� ���� SwapChain�� �����Ѵ�.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mScreenWidth;
	sd.BufferDesc.Height = mScreenHeight;
	sd.BufferDesc.RefreshRate.Numerator = 0; // ������� ����
	sd.BufferDesc.RefreshRate.Denominator = 0; // ������� ����
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1; // SwapChain���� ����ǥ��ȭ Ȱ��ȭ���� ����
	sd.SampleDesc.Quality = 0; // SwapChain���� ����ǥ��ȭ Ȱ��ȭ���� ����
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = !GetOptionEnabled(Option::Fullscreen);
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// ���� : ��ȯ �罽�� Command Queue�� �̿��ؼ� Flush�� �����Ѵ�.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	// ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϵ��� ��Ÿ�� ���� ������Ų��.
	mCurrentFence++;

	// �� ��Ÿ�� ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� ��Ÿ�� ������ GPU�� Signal() ��ɱ����� ��� ����� ó���ϱ�
	// �������� �������� �ʴ´�.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// GPU�� �� ��Ÿ�� ���������� ��ɵ��� �Ϸ��� ������ ��ٸ���.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// GPU�� ���� ��Ÿ�� ������ ���������� �̺�Ʈ�� �ߵ��Ѵ�.
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// GPU�� ���� ��Ÿ�� ������ ���������� ���ϴ� �̺�Ʈ�� ��ٸ���.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::GetCurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrentBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetMsaaBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		SWAP_CHAIN_BUFFER_COUNT,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
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

		LogOutputDisplayModes(output, mBackBufferFormat);

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

void D3DApp::SetGamma(float gamma)
{
	// ���� ������ Ǯ��ũ�� ��忡���� ����� �� �ִ�.
	if (!GetOptionEnabled(Option::Fullscreen))
		return;

	IDXGIOutput* output;
	DXGI_GAMMA_CONTROL_CAPABILITIES gammaCaps;
	DXGI_GAMMA_CONTROL gammaControl;

	ThrowIfFailed(mSwapChain->GetContainingOutput(&output));
	ThrowIfFailed(output->GetGammaControlCapabilities(&gammaCaps));

	gammaControl.Scale.Red = 1.0f;
	gammaControl.Scale.Green = 1.0f;
	gammaControl.Scale.Blue = 1.0f;

	gammaControl.Offset.Red = 1.0f;
	gammaControl.Offset.Green = 1.0f;
	gammaControl.Offset.Blue = 1.0f;

	for (UINT i = 0; i < gammaCaps.NumGammaControlPoints; ++i)
	{
		float p0 = gammaCaps.ControlPointPositions[i];
		float p1 = pow(p0, 1.0f / gamma); // ���� Ŀ�� ����

		gammaControl.GammaCurve[i].Red = p1;
		gammaControl.GammaCurve[i].Green = p1;
		gammaControl.GammaCurve[i].Blue = p1;
	}

	ThrowIfFailed(output->SetGammaControl(&gammaControl));
}

void D3DApp::SetBackgroundColor(float r, float g, float b, float a)
{
	mBackBufferClearColor.x = r;
	mBackBufferClearColor.y = g;
	mBackBufferClearColor.z = b;
	mBackBufferClearColor.w = a;
}

void D3DApp::ApplyOption(Option option)
{
	bool optionState = mOptions.test((int)option);

	switch (option)
	{
		case Option::Fullscreen:
			SetFullscreenState(optionState);
			break;
		case Option::Msaa:
			Set4xMsaaState(optionState);
			break;
	}
}